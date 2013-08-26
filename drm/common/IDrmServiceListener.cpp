/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <drm/drm_framework_common.h>
#include <drm/DrmInfoEvent.h>
#include "IDrmServiceListener.h"

using namespace android;

status_t BpDrmServiceListener::notify(const DrmInfoEvent& event) {
    Parcel data, reply;

    data.writeInterfaceToken(IDrmServiceListener::getInterfaceDescriptor());
    data.writeInt32(event.getUniqueId());
    data.writeInt32(event.getType());
    data.writeString8(event.getMessage());

    data.writeInt32(event.getCount());
    DrmInfoEvent::KeyIterator keyIt = event.keyIterator();
    while (keyIt.hasNext()) {
        String8 key = keyIt.next();
        data.writeString8(key);
        data.writeString8(event.get(key));
    }
    const DrmBuffer& value = event.getData();
    data.writeInt32(value.length);
    if (value.length > 0) {
        data.write(value.data, value.length);
    }

    remote()->transact(NOTIFY, data, &reply);
    return reply.readInt32();
}

IMPLEMENT_META_INTERFACE(DrmServiceListener, "drm.IDrmServiceListener");

status_t BnDrmServiceListener::onTransact(
        uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {

    switch (code) {
    case NOTIFY:
    {
        CHECK_INTERFACE(IDrmServiceListener, data, reply);
        int uniqueId = data.readInt32();
        int type = data.readInt32();
        const String8& message = data.readString8();

        DrmInfoEvent event(uniqueId, type, message);
        int size = data.readInt32();
        for (int index = 0; index < size; index++) {
            String8 key(data.readString8());
            String8 value(data.readString8());
            event.put(key, value);
        }
        int valueSize = data.readInt32();
        if (valueSize > 0) {
            char* valueData = new char[valueSize];
            data.read(valueData, valueSize);
            DrmBuffer drmBuffer(valueData, valueSize);
            event.setData(drmBuffer);
            delete[] valueData;
        }

        status_t status = notify(event);

        reply->writeInt32(status);

        return DRM_NO_ERROR;
    }
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

