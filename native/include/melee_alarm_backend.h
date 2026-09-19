#ifndef MELEE_ALARM_BACKEND_H
#define MELEE_ALARM_BACKEND_H
/* Call from the host after stopping game work, never inside an alarm callback
 * or with interrupts disabled. Cancels pending alarms and joins the worker. */
void melee_native_alarm_shutdown(void);
#endif
