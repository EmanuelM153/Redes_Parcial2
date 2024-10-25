#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SwitchHubCongestionSimulation");

int main(int argc, char *argv[]) {
    bool usarHubs = false;
    bool generarCongestion = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("usarHubs", "Usar hubs en vez de switches", useHubs);
    cmd.AddValue("generarCongestion", "Generar congestión en la red", generateCongestion);
    cmd.Parse(argc, argv);

    LogComponentEnable("V4Ping", LOG_LEVEL_INFO);

    NodeContainer devices;
    devices.Create(6);

    NodeContainer switches;
    switches.Create(3);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("50Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer deviceInterfaces;
    vector<Ptr<BridgeNetDevice>> bridgeDevices;

    for (uint32_t i = 0; i < switches.GetN(); ++i) {
        Ptr<Node> switchNode = switches.Get(i);
        Ptr<BridgeNetDevice> bridge = CreateObject<BridgeNetDevice>();
        switchNode->AddDevice(bridge);
        bridgeDevices.push_back(bridge);

        for (uint32_t j = 0; j < 2; ++j) {
            Ptr<Node> hostNode = devices.Get(2 * i + j);
            NetDeviceContainer link = csma.Install(NodeContainer(switchNode, hostNode));
            bridge->AddBridgePort(link.Get(0));
            deviceInterfaces.Add(link.Get(1));
        }
    }

    for (uint32_t i = 0; i < switches.GetN(); ++i) {
        for (uint32_t j = i + 1; j < switches.GetN(); ++j) {
            NetDeviceContainer link = csma.Install(NodeContainer(switches.Get(i), switches.Get(j)));
            bridgeDevices[i]->AddBridgePort(link.Get(0));
            bridgeDevices[j]->AddBridgePort(link.Get(1));
        }
    }

    InternetStackHelper internet;
    internet.Install(devices);
    Ipv4InterfaceContainer interfaces = ipv4.Assign(deviceInterfaces);

    for (uint32_t i = 0; i < interfaces.GetN(); ++i)
        cout << "Host " << i << " IP: " << interfaces.GetAddress(i) << endl;

    ApplicationContainer pingApps;
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        uint32_t remoteIndex = (i + 3) % devices.GetN();
        if (remoteIndex == i) remoteIndex = (i + 1) % devices.GetN();

        V4PingHelper ping(interfaces.GetAddress(remoteIndex));
        ping.SetAttribute("Verbose", BooleanValue(true));
        ping.SetAttribute("Interval", TimeValue(MilliSeconds(generarCongestion ? 10 : 100)));
        ping.SetAttribute("Size", UintegerValue(generarCongestion ? 1024 : 64));

        ApplicationContainer app = ping.Install(devices.Get(i));
        app.Start(Seconds(2.0));
        app.Stop(Seconds(10.0));
        pingApps.Add(app);
    }

    for (uint32_t i = 0; i < devices.GetN(); ++i)
        csma.EnablePcap("device_" + to_string(i) + ".pcap", devices.Get(i)->GetDevice(0), true, true);

    if (!usarHubs) {
        for (uint32_t i = 0; i < switches.GetN(); ++i) {
            Ptr<BridgeNetDevice> bridge = bridgeDevices[i];
            for (uint32_t k = 0; k < bridge->GetNBridgePorts(); ++k) {
                string filename = "switch_" + to_string(i) + "_port_" + to_string(k) + ".pcap";
                csma.EnablePcap(filename, bridge->GetBridgePort(k), true, true);
            }
        }
    }

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

