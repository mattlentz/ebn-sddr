#ifndef __BLUEDROID_BLUETOOTH_H__
#define __BLUEDROID_BLUETOOTH_H__

void stopBluetoothd();

/* Enable the bluetooth interface.
 *
 * Responsible for power on, bringing up HCI interface, and starting daemons.
 * Will block until the HCI interface and bluetooth daemons are ready to
 * use.
 */
void bt_enable( const int adapterID );

/* Disable the bluetooth interface.
 *
 * Responsbile for stopping daemons, pulling down the HCI interface, and
 * powering down the chip. Will block until power down is complete, and it
 * is safe to immediately call enable().
 */
void bt_disable( const int adapterID );

bool bt_is_enabled( const int adapterID );

#endif //__BLUEDROID_BLUETOOTH_H__
