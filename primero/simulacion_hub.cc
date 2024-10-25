#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

int main(int argc, char *argv[])
{
    string anchoBanda = "25Mbps";
    string algoritmoTCP = "cubic";
    uint32_t cantidadTrafico = 4;

    CommandLine cmd;
    cmd.AddValue("anchoBanda", "Ancho de banda del enlace central", anchoBanda);
    cmd.AddValue("traffic", "Cantidad de tráfico a enviar en MB", cantidadTrafico);
    cmd.AddValue("algoritmoTCP", "Algoritmo de Control de Congestión TCP (yeah, cubic, illinois)", algoritmoTCP);
    cmd.Parse(argc, argv);

    TypeId tcpTid;
    if (algoritmoTCP == "yeah") {
        tcpTid = TypeId::LookupByName("ns3::TcpYeah");
    } else if (algoritmoTCP == "cubic") {
        tcpTid = TypeId::LookupByName("ns3::TcpCubic");
    } else if (algoritmoTCP == "illinois") {
        tcpTid = TypeId::LookupByName("ns3::TcpIllinois");
    } else {
        NS_LOG_UNCOND("Algoritmo TCP inválido especificado");
        return 1;
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(tcpTid));

    NodeContainer nodos;
    nodos.Create(4);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue(anchoBanda));
    csma.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer dispositivos = csma.Install(nodos);

    InternetStackHelper stack;
    stack.Install(nodos);

    Ipv4AddressHelper address;
    address.SetBase("192.168.0.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(dispositivos);
    Ipv4Address ipNodo0 = interfaces.GetAddress(0);
    Ipv4Address ipNodo1 = interfaces.GetAddress(1);
    Ipv4Address ipNodo2 = interfaces.GetAddress(2);
    Ipv4Address ipNodo3 = interfaces.GetAddress(3);

    DceManagerHelper dceManager;
    dceManager.SetTaskManagerAttribute("FiberManagerType", StringValue("PthreadFiberManager"));
    dceManager.Install(nodos);
    DceApplicationHelper dce;
    uint32_t cantidadTraficoBytes = cantidadTrafico * 1000 * 1000;

    dce.SetStackSize(1 << 20);
    dce.SetBinary("iperf");
    dce.ResetArguments();
    dce.ResetEnvironment();
    dce.AddArgument("-s");
    ApplicationContainer servidorApp2 = dce.Install(nodos.Get(2));
    servidorApp2.Start(Seconds(1.0));

    dce.ResetArguments();
    dce.AddArgument("-s");
    ApplicationContainer servidorApp3 = dce.Install(nodos.Get(3));
    servidorApp3.Start(Seconds(1.0));



    dce.ResetArguments();
    dce.AddArgument("-c");
    ostringstream ossNode2;
    ossNode2 << ipNodo2;
    string servidorIpNodo2 = ossNode2.str();
    dce.AddArgument(servidorIpNodo2);
    dce.AddArgument("-n");
    dce.AddArgument(to_string(cantidadTraficoBytes));
    ApplicationContainer clienteApp0 = dce.Install(nodos.Get(0));
    clienteApp0.Start(Seconds(2.0));

    dce.ResetArguments();
    dce.AddArgument("-c");
    ostringstream ossNode3;
    ossNode3 << ipNodo3;
    string servidorIpNodo3 = ossNode3.str();
    dce.AddArgument(servidorIpNodo3);
    dce.AddArgument("-n");
    dce.AddArgument(to_string(cantidadTraficoBytes));
    ApplicationContainer clienteApp1 = dce.Install(nodos.Get(1));
    clienteApp1.Start(Seconds(2.0));

    csma.EnablePcap("hub-node0.pcap", dispositivos.Get(0), true, true);
    csma.EnablePcap("hub-node1.pcap", dispositivos.Get(1), true, true);
    csma.EnablePcap("hub-node2.pcap", dispositivos.Get(2), true, true);
    csma.EnablePcap("hub-node3.pcap", dispositivos.Get(3), true, true);

    Simulator::Stop(Seconds(300));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
