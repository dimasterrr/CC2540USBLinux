#include "BLE.h"
#include "HCICodes.h"

BLE::BLE():is_module_initialized(false), is_received(false) {

}

BLE::~BLE() {

}

std::vector<char> BLE::getGAPCommand(unsigned char cmd, std::vector<char> data) {
    std::vector<char> output;
    output.clear();
    output.push_back(COMMAND);
    switch (cmd) {
    case GAP_DEVICE_INITIALIZATION:
    {
        std::vector<char> irk(0x10, 0x00);
        std::vector<char> csrk(0x10, 0x00);
        std::vector<char> signal(0x0E, 0x00);
        unsigned char data_len = 0x30;
        signal[6] = 0x01;
        output.push_back(cmd); // GAP CMD
        output.push_back(0xFE);
        output.push_back(data_len);
        output.push_back(CENTRAL);
        output.push_back(0x03);
        output.insert(output.end(), irk.begin(), irk.end());
        output.insert(output.end(), csrk.begin(), csrk.end());
        output.insert(output.end(), signal.begin(), signal.end());
        break;
    }

    case GAP_DEVICE_DISCOVERY_REQUEST:
    {
        output.push_back(cmd); // GAP CMD
        output.push_back(0xFE);
        output.push_back(0x03); // Data length
        output.push_back(0x03); // Mode (all)
        output.push_back(0x01); // Enable name mode
        output.push_back(0x00); // Don't use white list
        break;
    }

    case GAP_ESTABLISH_LINK_REQUEST:
    {
        output.push_back(cmd); // GAP CMD
        output.push_back(0xFE);
        output.push_back(0x09); // Data length
        output.push_back(0x00); // Disable highDutyCycle
        output.push_back(0x00); // Don't use white list
        output.insert(output.end(), data.begin(), data.end()); // 1byte: AddrType, 6bytes: BinAddr
        break;
    }

    case GAP_TERMINATE_LINK_REQUEST:
    {
        output.push_back(cmd); // GAP CMD
        output.push_back(0xFE);
        output.push_back(0x03); // Data length
        //output.push_back(); // ConnHandle
        output.push_back(0x13); // Reason (Remote user terminated connection)
        break;
    }

    default:
        break;
    }

    return output;
}

std::vector<char> BLE::getGATTCommand(unsigned char cmd, std::vector<char> data) {
    std::vector<char> output;
    output.push_back(COMMAND);
    switch (cmd) {
    case GATT_DISCOVER_CHARACTERISTICS_BY_UUID:
    {
        output.push_back(cmd);
        output.insert(output.end(), data.begin(), data.end());
        break;
    }

    default:
        break;
    }
    return output;
}

void BLE::received(const char *data, unsigned int len) {
    is_received = false;
    printf("============================================\n");
    for(int i = 0; i<len; i++) {
        printf("%02x:", (unsigned char)*(data+i));
        if((i%16 == 0)&& i!=0) printf("\n");
    }
    printf("\nDone !\n");
    processHCIEvents(data,len);
    is_received = true;
}

bool BLE::setDongleAddress(const char *addr) {
    this->dongle_address.clear();
    for (int i = 0; i < ADDR_LEN; i++) {
        this->dongle_address.push_back(*(addr+i));
    }
#if defined (DEBUG)
    for (int i = 0; i < ADDR_LEN; i++) {
        printf("%02x:",(unsigned char)this->dongle_address[i]);
    }
#endif
    printf("\n");
    return true;
}

bool BLE::getDongleAddress(std::vector<char> &addr) {
    addr = this->dongle_address;
    return true;
}

bool *BLE::getModuleStatus() {
    return &is_module_initialized;
}

bool BLE::setModuleIRK(const char *irk) {
    this->irk.clear();
    for (int i = 0; i< IRK_LEN; i++) {
        this->irk.push_back(*(irk+i));
    }
    return true;
}

bool BLE::getModuleIRK(std::vector<char> &irk) {
    irk = this->irk;
    return true;
}

bool BLE::setModuleCSRK(const char *csrk) {
    this->csrk.clear();
    for (int i = 0; i< CSRK_LEN; i++) {
        this->csrk.push_back(*(csrk+i));
    }
    return true;
}

bool BLE::getModuleCSRK(std::vector<char> &csrk) {
    csrk = this->csrk;
    return true;
}

bool BLE::processHCIEvents(const char *data, unsigned int len) {
    bool status = false;
    if((unsigned char)data[0] == EVENT) {
#if defined (DEBUG)
        printf("============================================\n");
        printf("Event code: %02x\n",(unsigned char)data[1]);
#endif
        if((unsigned char)data[1] == VENDOR_SPECIFIC) {
#if defined (DEBUG)
            printf("Vendor Specific Event Code\n");
            printf("Data len: %d\n", data[2]);
            printf("Data Code: %02x%02x \n",(unsigned char)data[4], (unsigned char)data[3]);
#endif
            if((unsigned char)data[4] == 0x06) { // GAP event
#if defined (DEBUG)
                printf("GAP Event\n");
#endif
                if((unsigned char) data[3] == GAP_COMMAND_STATUS) {
#if defined (DEBUG)
                    printf("Got GAP Command Status\n");
#endif
                    if((unsigned char)data[7] == 0xFE) {
                        if((unsigned char)data[8] == 0x00) {
                            printf("GAP Device Initialization\n");
                        }
                    }
                } else if ((unsigned char)data[3] == GAP_DEVICE_INIT_DONE) {
#if defined (DEBUG)
                    printf("GAP Device Init Done\n");
#endif
                    if(data[5] == 0x00) { //Success
#if defined (DEBUG)
                        printf("Device initialized and ready\n");
#endif
                        processEventGAPDeviceDone(data, len);
                    }
                } else if ((unsigned char) data[3] == GAP_DEVICE_INFOMATION) {
#if defined (DEBUG)
                    printf("GAP Device Infomation\n");
#endif
                    processEventGAPDeviceInfomation(data, len);
                } else if ((unsigned char) data[3] == GAP_DEVICE_DISCOVERY) {
#if defined (DEBUG)
                    printf("GAP Device Discovery\n");
#endif
                    processEventGAPDeviceDiscovery(data, len);
                }
            }
        } else if ((unsigned char)data[1] == LOW_ENERGY) {

        } else if ((unsigned char)data[1] == COMMAND_COMPLETE) {
            printf("Command complete !\n");
#if defined (DEBUG)
            printf("Data len: %d\n", data[2]);
            printf("Data Code: %02x%02x \n",(unsigned char)data[4], (unsigned char)data[3]);
            printf("Data: ");
            for (int var = 5; var < len; var++) {
                printf("%02x:",(unsigned char) data[var]);
            }
            printf("\n");
#endif

        } else {
            printf("Error, can't process this event: %02x\n",(unsigned char)data[1]);
        }
    } else {

    }
    return status;
}

bool BLE::processEventGAPDeviceDone(const char *data, unsigned int len) {
    setDongleAddress(data+ADDR_POS);
    setModuleIRK(data+IRK_POS);
    setModuleCSRK(data+CSRK_POS);
    this->is_module_initialized = true;
    printf("Module status: %d\n", this->is_module_initialized);
    return true;
}

bool BLE::processEventGAPDeviceDiscovery(const char *data, unsigned int len) {
    if(data[DEVICE_DISCOVERY_STATUS_POS] == 0x00) {
        number_of_device_found = data[DEVICE_DISCOVERY_STATUS_POS + 1];
        if(number_of_device_found == 0) {
#if defined (DEBUG)
            printf("Device discovery done, no device found\n");
#endif
        } else {
#if defined (DEBUG)
            printf("Device discovery done, found %d device(s)\n", data[DEVICE_DISCOVERY_STATUS_POS + 1]);
#endif
//            devices_found.clear();
//            for(int i = 0; i<number_of_device_found; i++) {
//                BLEDevice device = BLEDevice();
//                device.setAddressType(data[DEVICE_DISCOVERY_STATUS_POS + 3 + i*8]);
//                device.setEventType(data[DEVICE_DISCOVERY_STATUS_POS + 2 + i*8]);
//                device.setAddress(data+DEVICE_DISCOVERY_STATUS_POS + 4+ i*8);
//                devices_found.push_back(device);
//            }
        }
    }
    return true;
}

bool BLE::processEventGAPLinkEstablished(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventGAPLinkTerminated(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventGAPDeviceInfomation(const char *data, unsigned int len) {
    // 0x05: status
    // 0x06: event type
    // 0x07: addr type
    // 0x08: addr (6 bytes)
    // 0x0E: rssi
    // 0x0F: dataLen
    // 0x10: dataField
    if (data[DEVICE_INFOMATION_STATUS_POS] == 0x00) {
        BLEDevice device = BLEDevice();
        device.setEventType(data[DEVICE_INFOMATION_STATUS_POS +1]);
        device.setAddressType(data[DEVICE_INFOMATION_STATUS_POS +2]);
        device.setAddress(data+DEVICE_INFOMATION_STATUS_POS +3);
        device.setRSSIValue(data[data+DEVICE_INFOMATION_STATUS_POS +9]);
        devices_found.push_back(device);
        printf("Found device :");
        unsigned char rssi = 0;
        std::vector<char> addr;
        device.getAddress(addr);
        for (int i = 0; i< 0x06; i++) {
            printf("");
        }
        device.getRSSIValue(rssi);
        printf("with RSSI: %02x\n", rssi);
    } else {

    }
    return true;
}

bool BLE::processEventGAPHCIExtCommandStatus(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventATTReadByTypeResponse(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventATTWriteResponse(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventATTHandleValueNotification(const char *data, unsigned int len) {
    return true;
}

bool BLE::processEventNoMatch(const char *data, unsigned int len) {
    return true;
}
