/*
 * Copyright (c) 2011 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Virtual USB device main routines.
 */
#include <errno.h>
#include <str.h>
#include <stdio.h>
#include <assert.h>
#include <async.h>
#include <devman.h>
#include <usbvirt/device.h>
#include <usbvirt/ipc.h>

#include <usb/debug.h>

/** Current device. */
static usbvirt_device_t *DEV = NULL;

/** Main IPC call handling from virtual host controller.
 *
 * @param iid Caller identification.
 * @param icall Initial incoming call.
 */
static void callback_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	assert(DEV != NULL);

	async_answer_0(iid, EOK);

	while (true) {
		ipc_callid_t callid;
		ipc_call_t call;

		callid = async_get_call(&call);
		bool processed = usbvirt_ipc_handle_call(DEV, callid, &call);
		if (!processed) {
			switch (IPC_GET_IMETHOD(call)) {
				case IPC_M_PHONE_HUNGUP:
					async_answer_0(callid, EOK);
					return;
				default:
					async_answer_0(callid, EINVAL);
					break;
			}
		}
	}
}

/** Connect the device to the virtual host controller.
 *
 * @param dev The virtual device to be (virtually) plugged in.
 * @param vhc_path Devman path to the virtual host controller.
 * @return Error code.
 */
int usbvirt_device_plug(usbvirt_device_t *dev, const char *vhc_path)
{
	int rc;
	devman_handle_t handle;

	if (DEV != NULL) {
		return ELIMIT;
	}

	rc = devman_device_get_handle(vhc_path, &handle, 0);
	if (rc != EOK) {
		return rc;
	}

	int hcd_phone = devman_device_connect(handle, 0);

	if (hcd_phone < 0) {
		return hcd_phone;
	}

	DEV = dev;
	dev->vhc_phone = hcd_phone;

	rc = async_connect_to_me(hcd_phone, 0, 0, 0, callback_connection);
	if (rc != EOK) {
		DEV = NULL;
	}

	return rc;
}

/** Disconnect the device from virtual host controller.
 *
 * @param dev Device to be disconnected.
 */
void usbvirt_device_unplug(usbvirt_device_t *dev)
{
	async_hangup(dev->vhc_phone);
}

/**
 * @}
 */