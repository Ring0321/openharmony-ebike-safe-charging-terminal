#include "smart_home_event.h"

#include <stdio.h>

#include "ohos_init.h"
#include "los_task.h"

static unsigned int event_queue_id;

#define EVENT_QUEUE_LENGTH 20
#define BUFFER_LEN         20

void smart_home_event_init(){
    unsigned int ret = LOS_OK;

    ret = LOS_QueueCreate("eventQ", EVENT_QUEUE_LENGTH, &event_queue_id, 0, BUFFER_LEN);
    if (ret != LOS_OK)
    {
        printf("Falied to create Message Queue ret:0x%x\n", ret);
        return;
    }
}
void smart_home_event_send(event_info_t *event)
{
    unsigned int ret = LOS_QueueWriteCopy(event_queue_id, event, sizeof(event_info_t), LOS_NO_WAIT);
    if (ret != LOS_OK) {
        printf("[safe_charge] event queue full, drop event=%d ret=0x%x\n", event->event, ret);
    }
}
int smart_home_event_wait(event_info_t *event,int timeoutMs){

    return LOS_QueueReadCopy(event_queue_id, event, sizeof(event_info_t),
        LOS_MS2Tick(timeoutMs));

}
