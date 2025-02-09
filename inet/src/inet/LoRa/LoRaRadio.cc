//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "LoRaRadio.h"
#include "inet/LoRaPhy/LoRaMedium.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/physicallayer/common/packetlevel/Radio.h"
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "inet/LoRaPhy/LoRaTransmitter.h"
#include "inet/LoRaPhy/LoRaReceiver.h"
#include "LoRaMacFrame_m.h"

namespace inet {

namespace physicallayer {

Define_Module(LoRaRadio);

simsignal_t LoRaRadio::minSNIRSignal = cComponent::registerSignal("minSNIR");
simsignal_t LoRaRadio::packetErrorRateSignal = cComponent::registerSignal("packetErrorRate");
simsignal_t LoRaRadio::bitErrorRateSignal = cComponent::registerSignal("bitErrorRate");
simsignal_t LoRaRadio::symbolErrorRateSignal = cComponent::registerSignal("symbolErrorRate");
simsignal_t LoRaRadio::droppedPacket = cComponent::registerSignal("droppedPacket");

LoRaRadio::~LoRaRadio()
{
    // NOTE: can't use the medium module here, because it may have been already deleted
    cModule *medium = getSimulation()->getModule(mediumModuleId);
    if (medium != nullptr)
        check_and_cast<IRadioMedium *>(medium)->removeRadio(this);
    cancelAndDelete(transmissionTimer);
    cancelAndDelete(switchTimer);
}

void LoRaRadio::initialize(int stage)
{
    PhysicalLayerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        transmissionTimer = new cMessage("transmissionTimer");
        antenna = check_and_cast<IAntenna *>(getSubmodule("antenna"));
        transmitter = check_and_cast<ITransmitter *>(getSubmodule("transmitter"));
        receiver = check_and_cast<IReceiver *>(getSubmodule("receiver"));
        medium = getModuleFromPar<IRadioMedium>(par("radioMediumModule"), this);
        mediumModuleId = check_and_cast<cModule *>(medium)->getId();
        upperLayerIn = gate("upperLayerIn");
        upperLayerOut = gate("upperLayerOut");
        radioIn = gate("radioIn");
        radioIn->setDeliverOnReceptionStart(true);
        separateTransmissionParts = par("separateTransmissionParts");
        separateReceptionParts = par("separateReceptionParts");
        displayCommunicationRange = par("displayCommunicationRange");
        displayInterferenceRange = par("displayInterferenceRange");
        WATCH(radioMode);
        WATCH(receptionState);
        WATCH(transmissionState);
        WATCH(receivedSignalPart);
        WATCH(transmittedSignalPart);
        iAmGateway = par("iAmGateway").boolValue();
    }
    else if (stage == INITSTAGE_PHYSICAL_LAYER) {
        medium->addRadio(this);
        switchTimer = new cMessage("switchTimer");
        parseRadioModeSwitchingTimes();
    }
    else if (stage == INITSTAGE_LAST) {
        EV_INFO << "Initialized " << getCompleteStringRepresentation() << endl;
    }
}

std::ostream& LoRaRadio::printToStream(std::ostream& stream, int level) const
{
    stream << static_cast<const cSimpleModule *>(this);
    if (level <= PRINT_LEVEL_TRACE)
        stream << ", antenna = " << printObjectToString(antenna, level + 1)
               << ", transmitter = " << printObjectToString(transmitter, level + 1)
               << ", receiver = " << printObjectToString(receiver, level + 1);
    return stream;
}

void LoRaRadio::setRadioMode(RadioMode newRadioMode)
{
    Enter_Method_Silent();
    if (newRadioMode < RADIO_MODE_OFF || newRadioMode > RADIO_MODE_SWITCHING)
        throw cRuntimeError("Unknown radio mode: %d", newRadioMode);
    else if (newRadioMode == RADIO_MODE_SWITCHING)
        throw cRuntimeError("Cannot switch manually to RADIO_MODE_SWITCHING");
    else if (radioMode == RADIO_MODE_SWITCHING || switchTimer->isScheduled())
        throw cRuntimeError("Cannot switch to a new radio mode while another switch is in progress");
    else if (newRadioMode != radioMode && newRadioMode != nextRadioMode) {
        simtime_t switchingTime = switchingTimes[radioMode][newRadioMode];
        if (switchingTime != 0)
            startRadioModeSwitch(newRadioMode, switchingTime);
        else
            completeRadioModeSwitch(newRadioMode);
    }
}

void LoRaRadio::parseRadioModeSwitchingTimes()
{
    const char *times = par("switchingTimes");

    char prefix[3];
    unsigned int count = sscanf(times, "%s", prefix);

    if (count > 2)
        throw cRuntimeError("Metric prefix should be no more than two characters long");

    double metric = 1;

    if (strcmp("s", prefix) == 0)
        metric = 1;
    else if (strcmp("ms", prefix) == 0)
        metric = 0.001;
    else if (strcmp("ns", prefix) == 0)
        metric = 0.000000001;
    else
        throw cRuntimeError("Undefined or missed metric prefix for switchingTimes parameter");

    cStringTokenizer tok(times + count + 1);
    unsigned int idx = 0;
    while (tok.hasMoreTokens()) {
        switchingTimes[idx / RADIO_MODE_SWITCHING][idx % RADIO_MODE_SWITCHING] = atof(tok.nextToken()) * metric;
        idx++;
    }
    if (idx != RADIO_MODE_SWITCHING * RADIO_MODE_SWITCHING)
        throw cRuntimeError("Check your switchingTimes parameter! Some parameters may be missed");
}

void LoRaRadio::startRadioModeSwitch(RadioMode newRadioMode, simtime_t switchingTime)
{
    EV_DETAIL << "Starting to change radio mode from " << getRadioModeName(radioMode) << " to " << getRadioModeName(newRadioMode) << endl;
    previousRadioMode = radioMode;
    radioMode = RADIO_MODE_SWITCHING;
    nextRadioMode = newRadioMode;
    emit(radioModeChangedSignal, radioMode);
    scheduleAt(simTime() + switchingTime, switchTimer);
}

void LoRaRadio::completeRadioModeSwitch(RadioMode newRadioMode)
{
    EV_INFO << "Radio mode changed from " << getRadioModeName(previousRadioMode) << " to " << getRadioModeName(newRadioMode) << endl;
    if (!isReceiverMode(newRadioMode) && receptionTimer != nullptr)
        abortReception(receptionTimer);
    if (!isTransmitterMode(newRadioMode) && transmissionTimer->isScheduled())
        abortTransmission();
    radioMode = previousRadioMode = nextRadioMode = newRadioMode;
    emit(radioModeChangedSignal, newRadioMode);
    updateTransceiverState();
    updateTransceiverPart();
}

const ITransmission *LoRaRadio::getTransmissionInProgress() const
{
    if (!transmissionTimer->isScheduled())
        return nullptr;
    else
        return static_cast<RadioFrame *>(transmissionTimer->getContextPointer())->getTransmission();
}

const ITransmission *LoRaRadio::getReceptionInProgress() const
{
    if (receptionTimer == nullptr)
        return nullptr;
    else
        return static_cast<RadioFrame *>(receptionTimer->getControlInfo())->getTransmission();
}

IRadioSignal::SignalPart LoRaRadio::getTransmittedSignalPart() const
{
    return transmittedSignalPart;
}

IRadioSignal::SignalPart LoRaRadio::getReceivedSignalPart() const
{
    return receivedSignalPart;
}

void LoRaRadio::handleMessageWhenDown(cMessage *message)
{
    if (message->getArrivalGate() == radioIn || isReceptionTimer(message))
        delete message;
    else
        OperationalBase::handleMessageWhenDown(message);
}

void LoRaRadio::handleMessageWhenUp(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else if (message->getArrivalGate() == upperLayerIn) {
        if (!message->isPacket()) {
            handleUpperCommand(message);
            delete message;
        }
        else
            handleUpperPacket(check_and_cast<cPacket *>(message));
    }
    else if (message->getArrivalGate() == radioIn) {
        if (!message->isPacket()) {
            handleLowerCommand(message);
            delete message;
        }
        else
            handleLowerPacket(check_and_cast<RadioFrame *>(message));
    }
    else
        throw cRuntimeError("Unknown arrival gate '%s'.", message->getArrivalGate()->getFullName());
}

void LoRaRadio::handleSelfMessage(cMessage *message)
{
    if (message == switchTimer)
        handleSwitchTimer(message);
    else if (message == transmissionTimer)
        handleTransmissionTimer(message);
    else if (isReceptionTimer(message))
        handleReceptionTimer(message);
    else
        throw cRuntimeError("Unknown self message");
}

void LoRaRadio::handleSwitchTimer(cMessage *message)
{
    completeRadioModeSwitch(nextRadioMode);
}

void LoRaRadio::handleTransmissionTimer(cMessage *message)
{
    if (message->getKind() == IRadioSignal::SIGNAL_PART_WHOLE)
        endTransmission();
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_PREAMBLE)
        continueTransmission();
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_HEADER)
        continueTransmission();
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_DATA)
        endTransmission();
    else
        throw cRuntimeError("Unknown self message");
}

void LoRaRadio::handleReceptionTimer(cMessage *message)
{
    if (message->getKind() == IRadioSignal::SIGNAL_PART_WHOLE)
        endReception(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_PREAMBLE)
        continueReception(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_HEADER)
        continueReception(message);
    else if (message->getKind() == IRadioSignal::SIGNAL_PART_DATA)
        endReception(message);
    else
        throw cRuntimeError("Unknown self message");
}

void LoRaRadio::handleUpperCommand(cMessage *message)
{
    if (message->getKind() == RADIO_C_CONFIGURE) {
        ConfigureRadioCommand *configureCommand = check_and_cast<ConfigureRadioCommand *>(message->getControlInfo());
        if (configureCommand->getRadioMode() != -1)
            setRadioMode((RadioMode)configureCommand->getRadioMode());
    }
    else
        throw cRuntimeError("Unsupported command");
}

void LoRaRadio::handleLowerCommand(cMessage *message)
{
    throw cRuntimeError("Unsupported command");
}

void LoRaRadio::handleUpperPacket(cPacket *packet)
{
    emit(LayeredProtocolBase::packetReceivedFromUpperSignal, packet);
    if (isTransmitterMode(radioMode)) {
        if (transmissionTimer->isScheduled())
            throw cRuntimeError("Received frame from upper layer while already transmitting.");
        if (separateTransmissionParts)
            startTransmission(packet, IRadioSignal::SIGNAL_PART_PREAMBLE);
        else
            startTransmission(packet, IRadioSignal::SIGNAL_PART_WHOLE);
    }
    else {
        EV_ERROR << "Radio is not in transmitter or transceiver mode, dropping frame." << endl;
        delete packet;
    }
}

void LoRaRadio::handleLowerPacket(RadioFrame *radioFrame)
{
    auto receptionTimer = createReceptionTimer(radioFrame);
    if (separateReceptionParts)
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_PREAMBLE);
    else
        startReception(receptionTimer, IRadioSignal::SIGNAL_PART_WHOLE);
}

bool LoRaRadio::handleNodeStart(IDoneCallback *doneCallback)
{
    // NOTE: we ignore radio mode switching during start
    completeRadioModeSwitch(RADIO_MODE_OFF);
    return PhysicalLayerBase::handleNodeStart(doneCallback);
}

bool LoRaRadio::handleNodeShutdown(IDoneCallback *doneCallback)
{
    // NOTE: we ignore radio mode switching and ongoing transmission during shutdown
    cancelEvent(switchTimer);
    if (transmissionTimer->isScheduled())
        abortTransmission();
    completeRadioModeSwitch(RADIO_MODE_OFF);
    return PhysicalLayerBase::handleNodeShutdown(doneCallback);
}

void LoRaRadio::handleNodeCrash()
{
    cancelEvent(switchTimer);
    if (transmissionTimer->isScheduled())
        abortTransmission();
    completeRadioModeSwitch(RADIO_MODE_OFF);
    PhysicalLayerBase::handleNodeCrash();
}

void LoRaRadio::startTransmission(cPacket *macFrame, IRadioSignal::SignalPart part)
{
    auto radioFrame = createRadioFrame(macFrame);
    auto transmission = radioFrame->getTransmission();
    transmissionTimer->setKind(part);
    transmissionTimer->setContextPointer(const_cast<RadioFrame *>(radioFrame));

    scheduleAt(transmission->getEndTime(part), transmissionTimer);
    EV_INFO << "Transmission started: " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    updateTransceiverState();
    updateTransceiverPart();
    //check_and_cast<LoRaMedium *>(medium)->fireTransmissionStarted(transmission);
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::transmissionStartedSignal, check_and_cast<const cObject *>(transmission));
}

void LoRaRadio::continueTransmission()
{
    auto previousPart = (IRadioSignal::SignalPart)transmissionTimer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame = static_cast<RadioFrame *>(transmissionTimer->getContextPointer());
    auto transmission = radioFrame->getTransmission();
    EV_INFO << "Transmission ended: " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << radioFrame->getTransmission() << endl;
    transmissionTimer->setKind(nextPart);
    scheduleAt(transmission->getEndTime(nextPart), transmissionTimer);
    EV_INFO << "Transmission started: " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << transmission << endl;
    updateTransceiverState();
    updateTransceiverPart();
}

void LoRaRadio::endTransmission()
{
    auto part = (IRadioSignal::SignalPart)transmissionTimer->getKind();
    auto radioFrame = static_cast<RadioFrame *>(transmissionTimer->getContextPointer());
    auto transmission = radioFrame->getTransmission();
    transmissionTimer->setContextPointer(nullptr);
    EV_INFO << "Transmission ended: " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    updateTransceiverState();
    updateTransceiverPart();
    //check_and_cast<LoRaMedium *>(medium)->fireTransmissionEnded(transmission);
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::transmissionEndedSignal, check_and_cast<const cObject *>(transmission));
}

void LoRaRadio::abortTransmission()
{
    auto part = (IRadioSignal::SignalPart)transmissionTimer->getKind();
    auto radioFrame = static_cast<RadioFrame *>(transmissionTimer->getContextPointer());
    auto transmission = radioFrame->getTransmission();
    transmissionTimer->setContextPointer(nullptr);
    EV_INFO << "Transmission aborted: " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << transmission << endl;
    EV_WARN << "Aborting ongoing transmissions is not supported" << endl;
    cancelEvent(transmissionTimer);
    updateTransceiverState();
    updateTransceiverPart();
}

RadioFrame *LoRaRadio::createRadioFrame(cPacket *packet) const
{
    RadioFrame *radioFrame = check_and_cast<RadioFrame *>(medium->transmitPacket(this, packet));
    ASSERT(radioFrame->getDuration() != 0);
    return radioFrame;
}

void LoRaRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    auto radioFrame = static_cast<RadioFrame *>(timer->getControlInfo());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();
// TODO: should be this, but it breaks fingerprints: if (receptionTimer == nullptr && isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
    if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
        auto transmission = radioFrame->getTransmission();
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, part);
        EV_INFO << "Reception started: " << (isReceptionAttempted ? "attempting" : "not attempting") << " " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        if (isReceptionAttempted)
        {
            receptionTimer = timer;
        }
    }
    else
        EV_INFO << "Reception started: ignoring " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    timer->setKind(part);
    scheduleAt(arrival->getEndTime(part), timer);
    updateTransceiverState();
    updateTransceiverPart();
    //check_and_cast<LoRaMedium *>(medium)->fireReceptionStarted(reception);
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::receptionStartedSignal, check_and_cast<const cObject *>(reception));
}

void LoRaRadio::continueReception(cMessage *timer)
{
    auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
    auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
    auto radioFrame = static_cast<RadioFrame *>(timer->getControlInfo());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();
    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime(previousPart) == simTime()) {
        auto transmission = radioFrame->getTransmission();
        bool isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, previousPart);
        EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        if (!isReceptionSuccessful)
        {
            receptionTimer = nullptr;
        }
        auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, nextPart);
        EV_INFO << "Reception started: " << (isReceptionAttempted ? "attempting" : "not attempting") << " " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
        if (!isReceptionAttempted)
        {
            receptionTimer = nullptr;
        }
    }
    else {
        EV_INFO << "Reception ended: ignoring " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
        EV_INFO << "Reception started: ignoring " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
    }
    timer->setKind(nextPart);
    scheduleAt(arrival->getEndTime(nextPart), timer);
    updateTransceiverState();
    updateTransceiverPart();
}

void LoRaRadio::endReception(cMessage *timer)
{
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto radioFrame = static_cast<RadioFrame *>(timer->getControlInfo());
    auto arrival = radioFrame->getArrival();
    auto reception = radioFrame->getReception();
    if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
    //if (isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
        auto transmission = radioFrame->getTransmission();
// TODO: this would draw twice from the random number generator in isReceptionSuccessful: auto isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, part);
        auto isReceptionSuccessful = medium->getReceptionDecision(this, radioFrame->getListening(), transmission, part)->isReceptionSuccessful();
        EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "successfully" : "unsuccessfully") << " for " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        auto macFrame = medium->receivePacket(this, radioFrame);
        if(isReceptionSuccessful)
        {
            emit(LayeredProtocolBase::packetSentToUpperSignal, macFrame);
            sendUp(macFrame);
        }
        else
        {
            emit(LoRaRadio::droppedPacket, 0);
            delete macFrame;
        }
        receptionTimer = nullptr;
    }
    else
        EV_INFO << "Reception ended: ignoring " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    updateTransceiverState();
    updateTransceiverPart();
    //check_and_cast<LoRaMedium *>(medium)->fireReceptionEnded(reception);
    check_and_cast<LoRaMedium *>(medium)->emit(IRadioMedium::receptionEndedSignal, check_and_cast<const cObject *>(reception));
    delete timer;
}

void LoRaRadio::abortReception(cMessage *timer)
{
    auto radioFrame = static_cast<RadioFrame *>(timer->getControlInfo());
    auto part = (IRadioSignal::SignalPart)timer->getKind();
    auto reception = radioFrame->getReception();
    EV_INFO << "Reception aborted: for " << (IRadioFrame *)radioFrame << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
    if (timer == receptionTimer)
    {
        receptionTimer = nullptr;
    }
    updateTransceiverState();
    updateTransceiverPart();
}

void LoRaRadio::captureReception(cMessage *timer)
{
    // TODO: this would be called when the receiver switches to a stronger signal while receiving a weaker one
    throw cRuntimeError("Not yet implemented");
}

void LoRaRadio::sendUp(cPacket *macFrame)
{
    auto indication = check_and_cast<const ReceptionIndication *>(macFrame->getControlInfo());
    emit(minSNIRSignal, indication->getMinSNIR());
    if (!std::isnan(indication->getPacketErrorRate()))
        emit(packetErrorRateSignal, indication->getPacketErrorRate());
    if (!std::isnan(indication->getBitErrorRate()))
        emit(bitErrorRateSignal, indication->getBitErrorRate());
    if (!std::isnan(indication->getSymbolErrorRate()))
        emit(symbolErrorRateSignal, indication->getSymbolErrorRate());
    EV_INFO << "Sending up " << macFrame << endl;
    send(macFrame, upperLayerOut);
}

cMessage *LoRaRadio::createReceptionTimer(RadioFrame *radioFrame) const
{
    cMessage *timer = new cMessage("receptionTimer");
    timer->setControlInfo(radioFrame);
    return timer;
}

bool LoRaRadio::isReceptionTimer(const cMessage *message) const
{
    return !strcmp(message->getName(), "receptionTimer");
}

bool LoRaRadio::isReceiverMode(IRadio::RadioMode radioMode) const
{
    return radioMode == RADIO_MODE_RECEIVER || radioMode == RADIO_MODE_TRANSCEIVER;
}

bool LoRaRadio::isTransmitterMode(IRadio::RadioMode radioMode) const
{
    return radioMode == RADIO_MODE_TRANSMITTER || radioMode == RADIO_MODE_TRANSCEIVER;
}

bool LoRaRadio::isListeningPossible() const
{
    const simtime_t now = simTime();
    const Coord position = antenna->getMobility()->getCurrentPosition();
    // TODO: use 2 * minInterferenceTime for lookahead? or maybe simply use 0 duration listening?
    const IListening *listening = receiver->createListening(this, now, now + 1E-12, position, position);
    const IListeningDecision *listeningDecision = medium->listenOnMedium(this, listening);
    bool isListeningPossible = listeningDecision->isListeningPossible();
    delete listening;
    delete listeningDecision;
    return isListeningPossible;
}

void LoRaRadio::updateTransceiverState()
{
    // reception state
    ReceptionState newRadioReceptionState;
    if (radioMode == RADIO_MODE_OFF || radioMode == RADIO_MODE_SLEEP || radioMode == RADIO_MODE_TRANSMITTER)
        newRadioReceptionState = RECEPTION_STATE_UNDEFINED;
    else if (receptionTimer && receptionTimer->isScheduled())
        newRadioReceptionState = RECEPTION_STATE_RECEIVING;
    else if (isListeningPossible())
        newRadioReceptionState = RECEPTION_STATE_BUSY;
    else
        newRadioReceptionState = RECEPTION_STATE_IDLE;
    if (receptionState != newRadioReceptionState) {
        EV_INFO << "Changing radio reception state from " << getRadioReceptionStateName(receptionState) << " to " << getRadioReceptionStateName(newRadioReceptionState) << "." << endl;
        receptionState = newRadioReceptionState;
        emit(receptionStateChangedSignal, newRadioReceptionState);
    }
    // transmission state
    TransmissionState newRadioTransmissionState;
    if (radioMode == RADIO_MODE_OFF || radioMode == RADIO_MODE_SLEEP || radioMode == RADIO_MODE_RECEIVER)
        newRadioTransmissionState = TRANSMISSION_STATE_UNDEFINED;
    else if (transmissionTimer->isScheduled())
        newRadioTransmissionState = TRANSMISSION_STATE_TRANSMITTING;
    else
        newRadioTransmissionState = TRANSMISSION_STATE_IDLE;
    if (transmissionState != newRadioTransmissionState) {
        EV_INFO << "Changing radio transmission state from " << getRadioTransmissionStateName(transmissionState) << " to " << getRadioTransmissionStateName(newRadioTransmissionState) << "." << endl;
        transmissionState = newRadioTransmissionState;
        emit(transmissionStateChangedSignal, newRadioTransmissionState);
    }
}

void LoRaRadio::updateTransceiverPart()
{
    IRadioSignal::SignalPart newReceivedPart = receptionTimer == nullptr ? IRadioSignal::SIGNAL_PART_NONE : (IRadioSignal::SignalPart)receptionTimer->getKind();
    if (receivedSignalPart != newReceivedPart) {
        EV_INFO << "Changing radio received signal part from " << IRadioSignal::getSignalPartName(receivedSignalPart) << " to " << IRadioSignal::getSignalPartName(newReceivedPart) << "." << endl;
        receivedSignalPart = newReceivedPart;
        emit(receivedSignalPartChangedSignal, receivedSignalPart);
    }
    IRadioSignal::SignalPart newTransmittedPart = !transmissionTimer->isScheduled() ? IRadioSignal::SIGNAL_PART_NONE : (IRadioSignal::SignalPart)transmissionTimer->getKind();
    if (transmittedSignalPart != newTransmittedPart) {
        EV_INFO << "Changing radio transmitted signal part from " << IRadioSignal::getSignalPartName(transmittedSignalPart) << " to " << IRadioSignal::getSignalPartName(newTransmittedPart) << "." << endl;
        transmittedSignalPart = newTransmittedPart;
        emit(transmittedSignalPartChangedSignal, transmittedSignalPart);
    }
}

double LoRaRadio::getCurrentTxPower()
{
    return currentTxPower;
}

void LoRaRadio::setCurrentTxPower(double txPower)
{
    currentTxPower = txPower;
}

} // namespace physicallayer

} // namespace inet
