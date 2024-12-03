Console.WriteLine("Using " + la_networkInterfaceHelper.getLibraryName() + " v" + la_networkInterfaceHelper.getLibraryVersion());
Console.WriteLine(la_networkInterfaceHelper.getLibraryCopyright());

displayInterfaces();

void displayInterfaces()
{
	Console.WriteLine("Available interfaces:");

	var obs = new Observer();
	la.networkInterface.NetworkInterfaceHelper.getInstance().registerObserver(obs);
	la.networkInterface.NetworkInterfaceHelper.getInstance().unregisterObserver(obs);
}

class Observer : la.networkInterface.NetworkInterfaceHelper.Observer
{
	public override void onInterfaceAdded(la.networkInterface.Interface intfc)
	{
		Console.WriteLine(intNum + ": " + intfc.id);
		Console.WriteLine("  Description:  " + intfc.description);
		Console.WriteLine("  Alias:        " + intfc.alias);
		Console.WriteLine("  MacAddress:   " + intfc.macAddress);
		Console.WriteLine("  Type:         " + intfc.type);
		Console.WriteLine("  Enabled:      " + (intfc.isEnabled ? "YES" : "NO"));
		Console.WriteLine("  Connected:    " + (intfc.isConnected ? "YES" : "NO"));
		Console.WriteLine("  Virtual:      " + (intfc.isVirtual ? "YES" : "NO"));
		if (!intfc.ipAddressInfos.IsEmpty)
		{
			Console.WriteLine("  IP Addresses:");
			foreach (var info in intfc.ipAddressInfos)
			{
				Console.WriteLine("    " + info.address.ToString() + " (" + info.netmask.ToString() + ") -> " + info.getNetworkBaseAddress().ToString() + " / " + info.getBroadcastAddress().ToString());
			}
		}
		if (!intfc.gateways.IsEmpty)
		{
			Console.WriteLine("  Gateways:");
			foreach (var ip in intfc.gateways)
			{
				Console.WriteLine("    " + ip.ToString());
			}
		}

		++intNum;
	}
	public override void onInterfaceRemoved(la.networkInterface.Interface intfc)
	{
	}
	public override void onInterfaceEnabledStateChanged(la.networkInterface.Interface intfc, bool isEnabled)
	{
	}
	public override void onInterfaceConnectedStateChanged(la.networkInterface.Interface intfc, bool isConnected)
	{
	}
	public override void onInterfaceAliasChanged(la.networkInterface.Interface intfc, string alias)
	{
	}
	public override void onInterfaceIPAddressInfosChanged(la.networkInterface.Interface intfc, IPAddressInfos ipAddressInfos)
	{
	}
	public override void onInterfaceGateWaysChanged(la.networkInterface.Interface intfc, Gateways gateways)
	{
	}

	private uint intNum = 1;
};
