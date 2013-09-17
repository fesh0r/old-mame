//============================================================
//
//  d3dintf.h - Direct3D 8/9 interface abstractions
//
//============================================================
//
//  Copyright Aaron Giles
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the
//  following conditions are met:
//
//    * Redistributions of source code must retain the above
//      copyright notice, this list of conditions and the
//      following disclaimer.
//    * Redistributions in binary form must reproduce the
//      above copyright notice, this list of conditions and
//      the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//    * Neither the name 'MAME' nor the names of its
//      contributors may be used to endorse or promote
//      products derived from this software without specific
//      prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
//  EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGE (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
//  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//============================================================

#ifndef __WIN_D3DINTF__
#define __WIN_D3DINTF__


//============================================================
//  CONSTANTS
//============================================================

#ifndef D3DCAPS2_DYNAMICTEXTURES
#define D3DCAPS2_DYNAMICTEXTURES 0x20000000L
#endif

#ifndef D3DPRESENT_DONOTWAIT
#define D3DPRESENT_DONOTWAIT 0x00000001L
#endif


#define D3DTSS_ADDRESSU       13
#define D3DTSS_ADDRESSV       14
#define D3DTSS_BORDERCOLOR    15
#define D3DTSS_MAGFILTER      16
#define D3DTSS_MINFILTER      17
#define D3DTSS_MIPFILTER      18
#define D3DTSS_MIPMAPLODBIAS  19
#define D3DTSS_MAXMIPLEVEL    20
#define D3DTSS_MAXANISOTROPY  21

namespace d3d
{
//============================================================
//  TYPE DEFINITIONS
//============================================================

struct base;
struct device;
struct surface;
struct texture;
struct vertex_buffer;
class effect;
typedef D3DXVECTOR4 vector;
typedef D3DMATRIX matrix;

//============================================================
//  Abstracted presentation parameters
//============================================================

struct present_parameters
{
	UINT BackBufferWidth;
	UINT BackBufferHeight;
	D3DFORMAT BackBufferFormat;
	UINT BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	DWORD MultiSampleQuality;
	D3DSWAPEFFECT SwapEffect;
	HWND hDeviceWindow;
	BOOL Windowed;
	BOOL EnableAutoDepthStencil;
	D3DFORMAT AutoDepthStencilFormat;
	DWORD Flags;
	UINT FullScreen_RefreshRateInHz;
	UINT PresentationInterval;
};


//============================================================
//  Abstracted device identifier
//============================================================

struct adapter_identifier
{
	char            Driver[512];
	char            Description[512];
	LARGE_INTEGER   DriverVersion;
	DWORD           VendorId;
	DWORD           DeviceId;
	DWORD           SubSysId;
	DWORD           Revision;
	GUID            DeviceIdentifier;
	DWORD           WHQLLevel;
};


//============================================================
//  Caps enumeration
//============================================================

enum caps_index
{
	CAPS_PRESENTATION_INTERVALS,
	CAPS_CAPS2,
	CAPS_DEV_CAPS,
	CAPS_SRCBLEND_CAPS,
	CAPS_DSTBLEND_CAPS,
	CAPS_TEXTURE_CAPS,
	CAPS_TEXTURE_FILTER_CAPS,
	CAPS_TEXTURE_ADDRESS_CAPS,
	CAPS_TEXTURE_OP_CAPS,
	CAPS_MAX_TEXTURE_ASPECT,
	CAPS_MAX_TEXTURE_WIDTH,
	CAPS_MAX_TEXTURE_HEIGHT,
	CAPS_STRETCH_RECT_FILTER,
	CAPS_MAX_PS30_INSN_SLOTS
};


//============================================================
//  Direct3D interfaces
//============================================================

struct interface
{
	HRESULT  (*check_device_format)(base *d3dptr, UINT adapter, D3DDEVTYPE devtype, D3DFORMAT adapterformat, DWORD usage, D3DRESOURCETYPE restype, D3DFORMAT format);
	HRESULT  (*check_device_type)(base *d3dptr, UINT adapter, D3DDEVTYPE devtype, D3DFORMAT format, D3DFORMAT backformat, BOOL windowed);
	HRESULT  (*create_device)(base *d3dptr, UINT adapter, D3DDEVTYPE devtype, HWND focus, DWORD behavior, present_parameters *params, device **dev);
	HRESULT  (*enum_adapter_modes)(base *d3dptr, UINT adapter, D3DFORMAT format, UINT index, D3DDISPLAYMODE *mode);
	UINT     (*get_adapter_count)(base *d3dptr);
	HRESULT  (*get_adapter_display_mode)(base *d3dptr, UINT adapter, D3DDISPLAYMODE *mode);
	HRESULT  (*get_adapter_identifier)(base *d3dptr, UINT adapter, DWORD flags, adapter_identifier *identifier);
	UINT     (*get_adapter_mode_count)(base *d3dptr, UINT adapter, D3DFORMAT format);
	HMONITOR (*get_adapter_monitor)(base *d3dptr, UINT adapter);
	HRESULT  (*get_caps_dword)(base *d3dptr, UINT adapter, D3DDEVTYPE devtype, caps_index which, DWORD *value);
	ULONG    (*release)(base *d3dptr);
};


//============================================================
//  Direct3DDevice interfaces
//============================================================

struct device_interface
{
	HRESULT (*begin_scene)(device *dev);
	HRESULT (*clear)(device *dev, DWORD count, const D3DRECT *rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil);
	HRESULT (*create_offscreen_plain_surface)(device *dev, UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, surface **surface);
	HRESULT (*create_texture)(device *dev, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, texture **texture);
	HRESULT (*create_vertex_buffer)(device *dev, UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, vertex_buffer **buf);
	HRESULT (*create_render_target)(device *dev, UINT width, UINT height, D3DFORMAT format, surface **surface);
	HRESULT (*draw_primitive)(device *dev, D3DPRIMITIVETYPE type, UINT start, UINT count);
	HRESULT (*end_scene)(device *dev);
	HRESULT (*get_raster_status)(device *dev, D3DRASTER_STATUS *status);
	HRESULT (*get_render_target)(device *dev, DWORD index, surface **surface);
	HRESULT (*get_render_target_data)(device *dev, surface *rendertarget, surface *destsurface);
	HRESULT (*present)(device *dev, const RECT *source, const RECT *dest, HWND override, RGNDATA *dirty, DWORD flags);
	ULONG   (*release)(device *dev);
	HRESULT (*reset)(device *dev, present_parameters *params);
	void    (*set_gamma_ramp)(device *dev, DWORD flags, const D3DGAMMARAMP *ramp);
	HRESULT (*set_render_state)(device *dev, D3DRENDERSTATETYPE state, DWORD value);
	HRESULT (*set_render_target)(device *dev, DWORD index, surface *surf);
	HRESULT (*set_stream_source)(device *dev, UINT number, vertex_buffer *vbuf, UINT stride);
	HRESULT (*set_texture)(device *dev, DWORD stage, texture *tex);
	HRESULT (*set_texture_stage_state)(device *dev, DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value);
	HRESULT (*set_vertex_format)(device *dev, D3DFORMAT format);
	HRESULT (*stretch_rect)(device *dev, surface *source, const RECT *srcrect, surface *dest, const RECT *dstrect, D3DTEXTUREFILTERTYPE filter);
	HRESULT (*test_cooperative_level)(device *dev);
};


//============================================================
//  Direct3DSurface interfaces
//============================================================

struct surface_interface
{
	HRESULT (*lock_rect)(surface *surf, D3DLOCKED_RECT *locked, const RECT *rect, DWORD flags);
	ULONG   (*release)(surface *tex);
	HRESULT (*unlock_rect)(surface *surf);
};


//============================================================
//  Direct3DTexture interfaces
//============================================================

struct texture_interface
{
	HRESULT (*get_surface_level)(texture *tex, UINT level, surface **surface);
	HRESULT (*lock_rect)(texture *tex, UINT level, D3DLOCKED_RECT *locked, const RECT *rect, DWORD flags);
	ULONG   (*release)(texture *tex);
	HRESULT (*unlock_rect)(texture *tex, UINT level);
};


//============================================================
//  Direct3DVertexBuffer interfaces
//============================================================

struct vertex_buffer_interface
{
	HRESULT (*lock)(vertex_buffer *vbuf, UINT offset, UINT size, VOID **data, DWORD flags);
	ULONG   (*release)(vertex_buffer *vbuf);
	HRESULT (*unlock)(vertex_buffer *vbuf);
};


//============================================================
//  Core D3D object
//============================================================

struct base
{
	// internal objects
	int                         version;
	void *                      d3dobj;
	HINSTANCE                   dllhandle;
	bool                        post_fx_available;
	HINSTANCE                   libhandle;

	// interface pointers
	interface               d3d;
	device_interface        device;
	surface_interface       surface;
	texture_interface       texture;
	vertex_buffer_interface vertexbuf;
};


//============================================================
//  PROTOTYPES
//============================================================

base *drawd3d9_init(void);

};

#endif
