#include "telemetry.h"

volatile Telemetry_Data_t g_telemetry_tx;
volatile Telemetry_Data_t g_telemetry_rx;

void Telemetry_Init(void)
{
    g_telemetry_tx.pack_voltage_raw = 4850;
    g_telemetry_tx.motor_temp_raw   = 350;
    g_telemetry_tx.system_state     = 0x00;
    g_telemetry_tx.error_code       = 0x01;
    g_telemetry_tx.counter          = 0;
}

void Telemetry_ProcessTx(void)
{
    CAN_Message_t tx_msg;

    tx_msg.id  = TELEMETRY_CAN_ID_STATUS;
    tx_msg.ide = 0;
    tx_msg.rtr = 0;
    tx_msg.dlc = TELEMETRY_PAYLOAD_SIZE;

    /* Big-Endian Paketleme */
    tx_msg.data[0] = (uint8_t)(g_telemetry_tx.pack_voltage_raw >> 8);
    tx_msg.data[1] = (uint8_t)(g_telemetry_tx.pack_voltage_raw & 0xFF);
    tx_msg.data[2] = (uint8_t)(g_telemetry_tx.motor_temp_raw >> 8);
    tx_msg.data[3] = (uint8_t)(g_telemetry_tx.motor_temp_raw & 0xFF);
    tx_msg.data[4] = g_telemetry_tx.system_state;
    tx_msg.data[5] = g_telemetry_tx.error_code;
    tx_msg.data[6] = g_telemetry_tx.counter;
    tx_msg.data[7] = 0xAA; /* Tail frame / checksum marker */

    CAN1_Transmit(&tx_msg);

    /* Test için simüle artış */
    g_telemetry_tx.counter++;
    g_telemetry_tx.pack_voltage_raw++;
}

uint8_t Telemetry_ProcessRx(void)
{
    CAN_Message_t rx_msg;

    if (CAN1_Receive(&rx_msg))
    {
        if (rx_msg.id == TELEMETRY_CAN_ID_STATUS)
        {
            /* Big-Endian Çözümleme (Unpacking) */
            g_telemetry_rx.pack_voltage_raw = (rx_msg.data[0] << 8) | rx_msg.data[1];
            g_telemetry_rx.motor_temp_raw   = (rx_msg.data[2] << 8) | rx_msg.data[3];
            g_telemetry_rx.system_state     = rx_msg.data[4];
            g_telemetry_rx.error_code       = rx_msg.data[5];
            g_telemetry_rx.counter          = rx_msg.data[6];
            return 1; /* Yeni veri alındı */
        }
    }
    return 0;
}
