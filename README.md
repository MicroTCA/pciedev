# pciedev
Simple PCIe driver builded on top of upciedev
It inherits basic PCIe functionality from below lying drivers (upciedev) 
that has an opportunity to work with any type of PCIe modules.
By default the pciedev binded to 0x10EE(Xilinx):0x0088 PCI Vendor:Device IDs.
It could be attached to any PCIe device sending command: 
echo “VendorID:DiviceID” >> /sys/bus/pci/drivers/pciedev/new_id  

Could be used as an example to build your own driver on top of upciedev.
