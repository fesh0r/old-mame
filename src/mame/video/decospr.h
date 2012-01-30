
typedef UINT16 (*decospr_priority_callback_func)(UINT16 pri);
typedef UINT16 (*decospr_colour_callback_func)(UINT16 col);

class decospr_device : public device_t
{
public:
	decospr_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	static void set_gfx_region(device_t &device, int gfxregion);
	static void set_pri_callback(device_t &device, decospr_priority_callback_func callback);
	static void set_col_callback(device_t &device, decospr_colour_callback_func callback);
	//void decospr_sprite_kludge(int x, int y);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, UINT16* spriteram, int sizewords, bool invert_flip = false );
	void draw_sprites(bitmap_rgb32 &bitmap, const rectangle &cliprect, UINT16* spriteram, int sizewords, bool invert_flip = false );
	void set_pri_callback(decospr_priority_callback_func callback);
	void set_col_callback(decospr_colour_callback_func callback);
	void set_gfxregion(int region) { m_gfxregion = region; };
	void set_alt_format(bool alt) { m_alt_format = alt; };
	void set_pix_mix_mask(UINT16 mask) { m_pixmask = mask; };
	void set_pix_raw_shift(UINT16 shift) { m_raw_shift = shift; };
	void alloc_sprite_bitmap();
	void inefficient_copy_sprite_bitmap(bitmap_rgb32 &bitmap, const rectangle &cliprect, UINT16 pri, UINT16 priority_mask, UINT16 colbase, UINT16 palmask, UINT8 alpha = 0xff);
	bitmap_ind16& get_sprite_temp_bitmap() { assert(m_sprite_bitmap.valid()); return m_sprite_bitmap; };

protected:
	virtual void device_start();
	virtual void device_reset();
	UINT8						m_gfxregion;
	decospr_priority_callback_func m_pricallback;
	decospr_colour_callback_func m_colcallback;
	bitmap_ind16 m_sprite_bitmap;// optional sprite bitmap (should be INDEXED16)
	bool m_alt_format;
	UINT16 m_pixmask;
	UINT16 m_raw_shift;

private:
	template<class _BitmapClass>
	void draw_sprites_common(_BitmapClass &bitmap, const rectangle &cliprect, UINT16* spriteram, int sizewords, bool invert_flip);
};

extern const device_type DECO_SPRITE;





