/*
xilprg is covered by the LGPL:

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

NeroJTAG support copyright (c) 2010 Chris McClelland
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string>
#include <libnero.h>
#include <libsync.h>
#include <libusbwrap.h>
#include <liberror.h>
#include <usb.h>
#include "xilprg.h"
#include "utils.h"
#include "nero.h"

// Exception thrown when problems occur.
//
class nero_exception : public std::exception {
	std::string msg;
public:
	explicit nero_exception(const char *s) : msg(s) { }
	virtual ~nero_exception() throw() { }
	virtual const char* what() const throw() {
		return msg.c_str();
	}
};

using namespace std;

nero::nero(const char *initStr) : m_devicePtr(NULL), m_neroHandle() {
	const char *ptr = initStr;
	while ( *ptr && *ptr != ':' ) {
		ptr++;
	}
	if ( *ptr == '\0' ) {
		throw nero_exception("Try nero:VID:PID");
	}
	ptr++;
	m_vidpid = ptr;
}

nero::~nero() {
}

// Find the NeroJTAG device, open it.
//
int nero::open() {
	const char *error;
	usbInitialise();
	if ( usbOpenDeviceVP(m_vidpid.c_str(), 1, 0, 0, &m_devicePtr, &error) ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
	if ( syncBulkEndpoints(m_devicePtr, SYNC_24, &error) ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
	if ( neroInitialise(m_devicePtr, &m_neroHandle, &error) ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}		
	return 0;
}

// Close the cable...drop the USB connection.
//
int nero::close() {
	const char *error;
	NeroStatus status = neroClose(&m_neroHandle, &error);
	usb_release_interface(m_devicePtr, 0);
	usb_close(m_devicePtr);
	if ( status != NERO_SUCCESS ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
	return 0;
}

// Shift data into and out of JTAG chain.
//   In pointer may be ALL_ZEROS (shift in zeros) or ALL_ONES (shift in ones).
//   Out pointer may be NULL (not interested in data shifted out of the chain).
//
void nero::shift(int numBits, void *const ptdi, void *const ptdo, int isLast) {
	const char *error;
	if ( neroShift(
		&m_neroHandle,
		numBits,
		(const u8 *)ptdi,
		(u8 *)ptdo,
		isLast==0 ? false : true,
		&error ) != NERO_SUCCESS )
	{
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
}

// Apply the supplied bit pattern to TMS, to move the TAP to a specific state.
//
void nero::tms_transition(u32 data, int numBits) {
	const char *error;
	if ( neroClockFSM(&m_neroHandle, data, (uint8)numBits, &error) != NERO_SUCCESS ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
}

// Cycle the TCK line for the given number of times.
//
void nero::tck_cycle(int numCycles) {
	const char *error;
	if ( neroClocks(&m_neroHandle, numCycles, &error) != NERO_SUCCESS ) {
		nero_exception ex(error);
		errFree(error);
		throw ex;
	}
}

// Get the cable description
//
int nero::get_description(string& desc) {
	desc = "MakeStuff NeroJTAG";
	return 0;
}
