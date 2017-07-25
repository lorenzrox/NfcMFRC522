DefinitionBlock ("Platform.aml", "DSDT", 5, "MSFT", "EDK2", 1)
{ 
    Scope (\_SB_)
    {
        Device (NFC0)
        {
            Name (_HID, "MFRC522")
            Name (_CID, "ACPI\MFRC522")
            Name(_CRS, ResourceTemplate()
            {
                SPISerialBus(0x2, PolarityLow, FourWireMode, 0x8, ControllerInitiated, 1000000, ClockPolarityLow, ClockPhaseFirst, "\\_SB.SPI1", 0, ResourceConsumer, , )
                GpioIO(Exclusive, PullDown, 0, 0, IoRestrictionNone, "\\_SB.GPI0", 0, ResourceConsumer, ) { 12 }
            })          
            Method (_STA, 0x0, NotSerialized)
            { 
                Return(0xf)
            }           
        }    
    }        
} 
