package inet.simulations;

import inet.applications.udpapp.UDPBasicApp;
import inet.node.inet.Router;
import inet.node.internetcloud.InternetCloud;
import inet.LoRaPhy.OpalLoRaRadioMedium;
import inet.LoraNode.OpalLoRaNode;
import inet.LoraNode.OpalLoRaGW;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;

@license(LGPL);
network LoRaNetworkOpalTest
{
    parameters:
        int numberOfNodes = default(1);
        int numberOfGateways = default(1);
        int networkSizeX = default(1000);
        int networkSizeY = default(1000);
        @display("bgb=1000,1000");
    submodules:
        loRaNodes[numberOfNodes]: OpalLoRaNode {
            @display("p=83.664,233.856");
            id = 0;
        }
        loRaGW[numberOfGateways]: OpalLoRaGW {
            @display("p=83.664,150.192;is=s");
            id = 1;
        }
        OpalLoRaRadioMedium: OpalLoRaRadioMedium {
            @display("p=791,93");
        }
        networkServer: StandardHost {
            parameters:
                @display("p=618,27");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=792,27");
        }
        internetCloud: InternetCloud {
            @display("p=362,27");
        }
        gwRouter[numberOfGateways]: Router {
            @display("p=268,27");
        }
        nsRouter: Router {
            @display("p=499,27");
        }
    connections:
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        for i=0..numberOfGateways-1 {
            internetCloud.pppg++ <--> Eth1G <--> gwRouter[i].pppg++;
            gwRouter[i].ethg++ <--> Eth1G <--> loRaGW[i].ethg++;
        }
}

