#ifndef LCDGPU2D_AXI_HPP
#define LCDGPU2D_AXI_HPP

#include <stdarg.h>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Font.hpp>
#include <touchgfx/TextProvider.hpp>
#include <touchgfx/TextureMapTypes.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/lcd/LCD.hpp>

#include <nema_blender.h>
#include <nema_graphics.h>

namespace touchgfx
{
#undef LCD

/**
 * @class LCDGPU2D_AXI LCDGPU2D_AXI.hpp
 *
 * @brief This class contains the various low-level drawing routines for drawing bitmaps.
 *
 *        This class contains the various low-level drawing routines for drawing bitmaps, texts
 *        and rectangles on 16 bits per pixel displays.
 *
 * @note All coordinates are expected to be in absolute coordinates!
 *
 * @see LCD
 */
class LCDGPU2D_AXI : public LCD
{
public:
    LCDGPU2D_AXI();

    virtual ~LCDGPU2D_AXI()
    {
    }

    /**
     * @fn virtual void LCDGPU2D_AXI::init();
     *
     * @brief Performs initialization.
     *
     *        Performs initialization.
     */
    virtual void init();

    /**
     * @fn void setFrameBufferFormat(touchgfx::Bitmap::BitmapFormat format);
     *
     * @brief Sets the framebuffer format to be used.
     *
     * @param format Format of the framebuffer.
     **/
    void setFrameBufferFormat(touchgfx::Bitmap::BitmapFormat format);

    /**
     * @fn void bindFrameBufferTexture();
     *
     * @brief Binds the framebuffer as destination texture.
     */
    virtual void bindFrameBufferTexture();

    /**
     * @fn virtual void LCDGPU2D_AXI::drawPartialBitmap(const Bitmap& bitmap, int16_t x, int16_t y, const Rect& rect, uint8_t alpha = 255, bool useOptimized = true);
     *
     * @brief Draws a portion of a bitmap.
     *
     *        Draws a portion of a bitmap.
     *
     * @param bitmap       The bitmap to draw.
     * @param x            The absolute x coordinate to place pixel (0, 0) on the screen.
     * @param y            The absolute y coordinate to place pixel (0, 0) on the screen.
     * @param rect         A rectangle describing what region of the bitmap is to be drawn.
     * @param alpha        Optional alpha value. Default is 255 (solid).
     * @param useOptimized if false, do not attempt to substitute (parts of) this bitmap with
     *                     faster fillrects.
     */
    virtual void drawPartialBitmap(const Bitmap& bitmap, int16_t x, int16_t y, const Rect& rect, uint8_t alpha = 255, bool useOptimized = true);

    /**
     * @fn virtual void LCDGPU2D_AXI::blitCopy(const uint16_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha, bool hasTransparentPixels);
     *
     * @brief Blits a 2D source-array to the framebuffer.
     *
     *        Blits a 2D source-array to the framebuffer perfoming alpha-blending (and
     *        tranparency keying) as specified Performs a software blend if HAL does not
     *        support BLIT_COPY_WITH_ALPHA and alpha != 255.
     *
     * @param sourceData           The source-array pointer (points to the beginning of the
     *                             data). The sourceData must be stored as 16-bits RGB565
     *                             values.
     * @param source               The location and dimension of the source.
     * @param blitRect             A rectangle describing what region is to be drawn.
     * @param alpha                The alpha value to use for blending (255 = solid, no blending)
     * @param hasTransparentPixels If true, this data copy contains transparent pixels and
     *                             require hardware support for that to be enabled.
     */
    virtual void blitCopy(const uint16_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha, bool hasTransparentPixels);

    /**
     * @fn virtual void LCDGPU2D_AXI::blitCopy(const uint8_t* sourceData, Bitmap::BitmapFormat sourceFormat, const Rect& source, const Rect& blitRect, uint8_t alpha, bool hasTransparentPixels);
     *
     * @brief Blits a 2D source-array to the framebuffer while converting the format.
     *
     *        Blits a 2D source-array to the framebuffer perfoming alpha-blending (and
     *        tranparency keying) as specified. LCDGPU2D_AXI supports source data
     *        formats: RGB565 and ARGB8888.
     *
     * @param sourceData           The source-array pointer (points to the beginning of the
     *                             data). The sourceData must be stored in a format suitable for
     *                             the selected display.
     * @param sourceFormat         The bitmap format used in the source data.
     * @param source               The location and dimension of the source.
     * @param blitRect             A rectangle describing what region is to be drawn.
     * @param alpha                The alpha value to use for blending (255 = solid, no blending)
     * @param hasTransparentPixels If true, this data copy contains transparent pixels and
     *                             require hardware support for that to be enabled.
     */
    virtual void blitCopy(const uint8_t* sourceData, Bitmap::BitmapFormat sourceFormat, const Rect& source, const Rect& blitRect, uint8_t alpha, bool hasTransparentPixels);

    /**
     * @fn virtual uint16_t* LCDGPU2D_AXI::copyFrameBufferRegionToMemory(const Rect& visRegion, const Rect& absRegion, const BitmapId bitmapId);
     *
     * @brief Copies part of the frame buffer region to memory.
     *
     *        Copies part of the framebuffer region to memory. The memory is given as BitmapId,
     *        which can be BITMAP_ANIMATION_STORAGE. The two regions given are the visible region
     *        and the absolute region on screen. This is used to copy only a part of an area. This
     *        might be the case if a SnapshotWidget is placed inside a Container where parts of the
     *        SnapshowWidget is outside the area defined by the Container. The visible region must
     *        be completely inside the absolute region.
     *
     * @note There is only one instance of animation storage. The content of the animation storage
     *       outside the given region is undefined.
     *
     * @param visRegion The visible region.
     * @param absRegion The absolute region.
     * @param bitmapId  Identifier for the bitmap.
     *
     * @returns Null if it fails, else a pointer to the data in the given bitmap.
     *
     * @see blitCopy
     */
    virtual uint16_t* copyFrameBufferRegionToMemory(const Rect& visRegion, const Rect& absRegion, const BitmapId bitmapId);

    /**
     * Copies part of the framebuffer to memory. The memory is assumed to have the same format as
     * the framebuffer. The two regions given are the visible region and the absolute region on
     * screen. This is used to copy only a part of the framebuffer. This might be the case if a
     * SnapshotWidget is placed inside a Container where parts of the SnapshowWidget is outside the
     * area defined by the Container. The visible region must be completely inside the absolute
     * region.
     *
     * @param           visRegion   The visible region.
     * @param           absRegion   The absolute region.
     * @param [in,out]  dst         Destination memory in same format as the framebuffer.
     * @param           dstWidth    Width of the destination.
     * @param           dstHeight   Height of the destination.
     *
     * @return  The rect that was actually copied to the destination buffer.
     *
     * @see blitCopy, copyFrameBufferRegionToMemory(const Rect&, const Rect&, const BitmapId)
     *
     * @note    There is only one instance of animation storage. The content of the bitmap data
     *          /animation storage outside the given region is left untouched.
     */
    virtual Rect copyFrameBufferRegionToMemory(const Rect& visRegion, const Rect& absRegion, uint8_t* dst, int16_t dstWidth, int16_t dstHeight);

    /**
     * Copies part of the displayed framebuffer to current framebuffer.
     * The region given is the absolute region on screen.
     *
     * @param  region               A rectangle describing what region of the displayed framebuffer
     *                              is to be copied to the framebuffer.
     *
     * @note    The copy is performed only when double buffering is enabled. Otherwise the given
     *          region in current framebuffer is left untouched.
     */
    virtual void copyAreaFromTFTToClientBuffer(const Rect& region);

    /**
     * @fn virtual void LCDGPU2D_AXI::fillRect(const Rect& rect, colortype color, uint8_t alpha = 255);
     *
     * @brief Draws a filled rectangle in the specified color.
     *
     *        Draws a filled rectangle in the specified color.
     *
     * @param rect  The rectangle to draw in absolute coordinates.
     * @param color The rectangle color.
     * @param alpha The rectangle opacity (255=solid)
     */
    virtual void fillRect(const Rect& rect, colortype color, uint8_t alpha = 255);

    /**
     * Draws a filled rectangle in the framebuffer in the specified color and opacity. By default
     * the rectangle will be drawn as a solid box. The rectangle can be drawn with transparency by
     * specifying alpha from 0=invisible to 255=solid.
     *
     * @param [in]  destination The start of the memory area to fill.
     * @param       pixelStride The pixel stride, i.e. number of pixels in a row.
     * @param       rect        The rectangle to fill absolute coordinates.
     * @param       color       The rectangle color.
     * @param       alpha       The rectangle opacity, from 0=invisible to 255=solid.
     *
     * @note    The pixelStride is rounded up to nearest whole bytes for displays with more than one
     *          pixel per byte (LCD1bpp, LCD2bpp and LCD4bpp)
     */
    virtual void fillBuffer(uint8_t* const destination, uint16_t pixelStride, const Rect& rect, const colortype color, const uint8_t alpha);

    /**
     * @fn virtual uint8_t LCDGPU2D_AXI::bitDepth() const
     *
     * @brief Number of bits per pixel used by the display.
     *
     *        Number of bits per pixel used by the display.
     *
     * @return The number of bits per pixel.
     */
    virtual uint8_t bitDepth() const
    {
        return (format == Bitmap::ARGB8888) ? 32 : (format == Bitmap::RGB888) ? 24
                                                                              : 16;
    }

    /**
     * @fn virtual Bitmap::BitmapFormat LCDGPU2D_AXI::framebufferFormat() const
     *
     * @brief Framebuffer format used by the display
     *
     *        Framebuffer format used by the display
     *
     * @return Bitmap::ARGB8888.
     */
    virtual Bitmap::BitmapFormat framebufferFormat() const
    {
        return format;
    }

    /**
     * @fn virtual uint16_t LCDGPU2D_AXI::framebufferStride() const
     *
     * @brief Framebuffer stride in bytes
     *
     *        Framebuffer stride in bytes. The distance (in bytes) from the start of one
     *        framebuffer row, to the next.
     *
     * @return The number of bytes in one framebuffer row.
     */
    virtual uint16_t framebufferStride() const
    {
        return getFramebufferStride();
    }

    /**
     * @fn FORCE_INLINE_FUNCTION uint16_t LCDGPU2D_AXI::getFramebufferStride() const
     *
     * @brief Framebuffer stride in bytes
     *
     *        Framebuffer stride in bytes. The distance (in bytes) from the start of one
     *        framebuffer row, to the next.
     *
     * @return The number of bytes in one framebuffer row.
     */
    FORCE_INLINE_FUNCTION uint16_t getFramebufferStride() const
    {
        assert(HAL::FRAME_BUFFER_WIDTH > 0 && "HAL has not been initialized yet");

        return HAL::FRAME_BUFFER_WIDTH * ((format == Bitmap::ARGB8888) ? 4 : ((format == Bitmap::RGB888) ? 3 : 2));
    }

    /**
     * Generates a color representation to be used on the LCD, based on 24 bit RGB values.
     *
     * @param  red   Value of the red part (0-255).
     * @param  green Value of the green part (0-255).
     * @param  blue  Value of the blue part (0-255).
     *
     * @return The color representation depending on LCD color format.
     */
    FORCE_INLINE_FUNCTION uint16_t getNativeColorFromRGB(uint8_t red, uint8_t green, uint8_t blue)
    {
        if (format == Bitmap::ARGB8888 || format == Bitmap::RGB888)
        {
            return (0xFF << 24) | (red << 16) | (green << 8) | (blue);
        }
        else
        {
            return ((red << 8) & 0xF800) | ((green << 3) & 0x07E0) | ((blue >> 3) & 0x001F);
        }
    }

    /**
     * Generates a color representation to be used on the LCD, based on 24 bit RGB values.
     *
     * @param  color The color.
     *
     * @return The color representation depending on LCD color format.
     */
    FORCE_INLINE_FUNCTION uint16_t getNativeColor(colortype color)
    {
        return getNativeColorFromRGB(Color::getRed(color), Color::getGreen(color), Color::getBlue(color));
    }

    FORCE_INLINE_FUNCTION void enableTextureMapperAll()
    {
    }

protected:
    /**
     * @fn virtual void LCDGPU2D_AXI::drawGlyph(uint16_t* wbuf16, Rect widgetArea, int16_t x, int16_t y, uint16_t offsetX, uint16_t offsetY, const Rect& invalidatedArea, const GlyphNode* glyph, const uint8_t* glyphData, uint8_t dataFormatA4, colortype color, uint8_t bitsPerPixel, uint8_t alpha, TextRotation rotation);
     *
     * @brief Private version of draw-glyph with explicit destination buffer pointer argument.
     *
     *        Private version of draw-glyph with explicit destination buffer pointer argument.
     *        For all parameters (except the buffer pointer) see the public version of
     *        drawGlyph().
     *
     * @param [in] wbuf16     The destination (frame) buffer to draw to.
     * @param widgetArea      The canvas to draw the glyph inside.
     * @param x               Horizontal offset to start drawing the glyph.
     * @param y               Vertical offset to start drawing the glyph.
     * @param offsetX         Horizontal offset in the glyph to start drawing from.
     * @param offsetY         Vertical offset in the glyph to start drawing from.
     * @param invalidatedArea The area to draw within.
     * @param glyph           Specifications of the glyph to draw.
     * @param glyphData       Data containing the actual glyph (dense format)
     * @param dataFormatA4    The glyph is saved using ST A4 format.
     * @param color           The color of the glyph.
     * @param bitsPerPixel    Bit depth of the glyph.
     * @param alpha           The transparency of the glyph.
     * @param rotation        Rotation to do before drawing the glyph.
     */
    virtual void drawGlyph(uint16_t* wbuf16, Rect widgetArea, int16_t x, int16_t y, uint16_t offsetX, uint16_t offsetY, const Rect& invalidatedArea, const GlyphNode* glyph, const uint8_t* glyphData, uint8_t dataFormatA4, colortype color, uint8_t bitsPerPixel, uint8_t alpha, TextRotation rotation);

    /**
     * @fn virtual void LCD::drawTextureMapTriangle(const DrawingSurface& dest, const Point3D* vertices, const TextureSurface& texture, const Rect& absoluteRect, const Rect& dirtyAreaAbsolute, RenderingVariant renderVariant, uint8_t alpha = 255, uint16_t subDivisionSize = 12);
     *
     * @brief Texture map triangle. Draw a perspective correct texture mapped triangle. The
     *        vertices describes the surface, the x,y,z coordinates and the u,v coordinates of
     *        the texture. The texture contains the image data to be drawn The triangle line
     *        will be placed and clipped using the absolute and dirty rectangles The alpha will
     *        determine how the triangle should be alpha blended. The subDivisionSize will
     *        determine the size of the piecewise affine texture mapped portions of the
     *        triangle.
     *
     * @param dest              The description of where the texture is drawn - can be used to
     *                          issue a draw off screen.
     * @param vertices          The vertices of the triangle.
     * @param texture           The texture.
     * @param absoluteRect      The containing rectangle in absolute coordinates.
     * @param dirtyAreaAbsolute The dirty area in absolute coordinates.
     * @param renderVariant     The render variant - includes the algorithm and the pixel format.
     * @param alpha             the alpha. Default = 255.
     * @param subDivisionSize   the size of the subdivisions of the scan line. Default = 12.
     */
    virtual void drawTextureMapTriangle(const DrawingSurface& dest,
                                        const Point3D* vertices,
                                        const TextureSurface& texture,
                                        const Rect& absoluteRect,
                                        const Rect& dirtyAreaAbsolute,
                                        RenderingVariant renderVariant,
                                        uint8_t alpha = 255,
                                        uint16_t subDivisionSize = 12);

    /**
     * @fn virtual void LCD::drawTextureMapQuad(const DrawingSurface& dest, const Point3D* vertices, const TextureSurface& texture, const Rect& absoluteRect, const Rect& dirtyAreaAbsolute, RenderingVariant renderVariant, uint8_t alpha = 255, uint16_t subDivisionSize = 12);
     *
     * @brief Texture map quad. Draw a perspective correct texture mapped quad. The
     *        vertices describes the surface, the x,y,z coordinates and the u,v coordinates of
     *        the texture. The texture contains the image data to be drawn The quad line
     *        will be placed and clipped using the absolute and dirty rectangles The alpha will
     *        determine how the quad should be alpha blended. The subDivisionSize will
     *        determine the size of the piecewise affine texture mapped portions of the
     *        quad.
     *
     * @param dest              The description of where the texture is drawn - can be used to
     *                          issue a draw off screen.
     * @param vertices          The vertices of the quad.
     * @param texture           The texture.
     * @param absoluteRect      The containing rectangle in absolute coordinates.
     * @param dirtyAreaAbsolute The dirty area in absolute coordinates.
     * @param renderVariant     The render variant - includes the algorithm and the pixel format.
     * @param alpha             the alpha. Default = 255.
     * @param subDivisionSize   the size of the subdivisions of the scan line. Default = 12.
     */
    virtual void drawTextureMapQuad(const DrawingSurface& dest,
                                    const Point3D* vertices,
                                    const TextureSurface& texture,
                                    const Rect& absoluteRect,
                                    const Rect& dirtyAreaAbsolute,
                                    RenderingVariant renderVariant,
                                    uint8_t alpha = 255,
                                    uint16_t subDivisionSize = 12);

    /**
     * Gets pointer to object that can draw a scan line which allows for highly specialized
     * and optimized implementation.
     *
     * @param  texture       The texture Surface.
     * @param  renderVariant The render variant.
     * @param  alpha         The global alpha.
     *
     * @return Null if it fails, else the pointer to the texture mapper draw scan line
     *         object.
     */
    virtual DrawTextureMapScanLineBase* getTextureMapperDrawScanLine(const TextureSurface& texture, RenderingVariant renderVariant, uint8_t alpha);

private:
    touchgfx::Bitmap::BitmapFormat format; // Framebuffer format
     static const uint16_t BLOCK_SIZE = 1024U;
    static const uint16_t MAX_DICT_SIZE = 512U;
    uint8_t entryBuffer[64]; // Having 64byte max entry sizes entails 1 + 2 + ... + 63 + 64 = 2080px size image blocks

    struct LZW9DictionaryEntry
    {
        uint8_t character;    // Current character of the entry
        uint8_t length;       // Remaining length of the entry
        uint16_t prefixIndex; // Index to previous character
    };

    LZW9DictionaryEntry lzw9Dictionary[MAX_DICT_SIZE];

    /**
     * @fn int UVtoCorner(float u, float v);
     *
     * @brief Convert texture coordinate (u,v) to corner index.
     *
     * @param u The horizontal texture coordinate
     * @param v The vertical texture coordinate
     */
    int UVtoCorner(float u, float v);

    /**
     * @fn int UVtoCornerQuad(float u, float v);
     *
     * @brief Convert texture coordinate (u,v) to corner index for quads drawing.
     *
     * @param u The horizontal texture coordinate
     * @param v The vertical texture coordinate
     */
    int UVtoCornerQuad(float u, float v);

    //Software/DMA2D implementations for L8 bitmaps

    /**
     * Blits a 2D indexed 8-bit source to the framebuffer performing alpha-blending per
     * pixel as specified if indexed format is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_ARGB8888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blends a pixel if alpha is applied to the image or the pixel itself, otherwise returns the solid color.
     *
     * @param  imagePixel The source pixel color data stored as 32-bits format.
     * @param  fbPixel    The framebuffer pixel color data stored as 16-bits format.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     * @param  alphapix   The alpha value to use for blending applied to the current pixel (255 =
     *                    solid, no blending)
     *
     * @return The blended pixel color data.
     */
    uint16_t blendPixelARGB8888_16BPP(uint32_t imagePixel, uint16_t fbPixel, int alpha, uint8_t alphapix);

    /**
     * Blits a L4-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The L4-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_L4_ARGB8888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a RLE-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The RLE-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RLE_ARGB8888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a LZW9-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The LZW9-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_LZW9_ARGB8888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RGB888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blends a pixel if alpha is applied to the image.
     *
     * @param  imagePixel The source pixel color data pointer stored as 8-bits format.
     * @param  fbPixel    The framebuffer pixel color data stored as 16-bits format.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     * @param  ialpha     Opposite of the alpha value used for blending applied to the current pixel (0 =
     *                    solid, no blending)
     */
    void blendPixelRGB888_16BPP(const uint8_t* imagePixel, uint16_t* const fbPixel, uint8_t alpha, uint8_t ialpha) const;

    /**
     * Blits a L4-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The L4-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_L4_RGB888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a RLE-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The RLE-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RLE_RGB888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a LZW9-compressed 2D indexed 8-bit source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The LZW9-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_LZW9_RGB888_16BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RGB888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blends a pixel if alpha is applied to the image.
     *
     * @param  imagePixel The source pixel color data pointer stored as 8-bits format.
     * @param  fbPixel    The framebuffer pixel color data pointer stored as 32-bits format.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blendPixelRGB888_32BPP(const uint8_t* imagePixel, uint32_t* fbPixel, const uint8_t alpha) const;

    /**
     * Blits a L4-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The L4-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_L4_RGB888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a RLE-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The RLE-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RLE_RGB888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a LZW9-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_RGB888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The LZW9-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (RGB888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_LZW9_RGB888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_ARGB8888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blends a pixel if alpha is applied to the image or the pixel itself, otherwise returns the solid color.
     *
     * @param  imagePixel The source pixel color data pointer stored as 8-bits format.
     * @param  fbPixel    The framebuffer pixel color data pointer stored as 32-bits format.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     * @param  alphapix   The alpha value to use for blending applied to the current pixel (255 =
     *                    solid, no blending)
     */
    void blendPixelARGB8888_32BPP(const uint8_t* imagePixel, uint32_t* fbPixel, const uint8_t alpha, const uint8_t alphapix) const;

    /**
     * Blits a L4-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The L4-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_L4_ARGB8888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a RLE-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The RLE-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_RLE_ARGB8888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a LZW9-compressed 2D indexed 8-bit source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if L8_ARGB8888 is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The LZW9-compressed source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  clutData   The source-clut pointer (points to the beginning of the CLUT color
     *                    format and size data followed by colors entries stored as 32-
     *                    bits (ARGB8888) format.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyL8_LZW9_ARGB8888_32BPP(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    //Software/DMA2D implementations for RGB888 bitmaps

    /**
     * Blits a 2D indexed RGB888 source to the framebuffer performing alpha-blending per
     * pixel as specified if indexed format is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyRGB888(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed RGB888 source to the 16bpp framebuffer performing alpha-blending per
     * pixel as specified if indexed format is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyRGB888_16BPP(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    /**
     * Blits a 2D indexed RGB888 source to the 32bpp framebuffer performing alpha-blending per
     * pixel as specified if indexed format is not supported by the DMA a software blend is
     * performed.
     *
     * @param  sourceData The source-indexes pointer (points to the beginning of the data). The
     *                    sourceData must be stored as 8- bits indexes.
     * @param  source     The location and dimensions of the source.
     * @param  blitRect   A rectangle describing what region is to be drawn.
     * @param  alpha      The alpha value to use for blending applied to the whole image (255 =
     *                    solid, no blending)
     */
    void blitCopyRGB888_32BPP(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha);

    //TextureMapper functions and classes

    FORCE_INLINE_FUNCTION static uint32_t expandRgb565(uint16_t c)
    {
        return ((c & 0x07E0) << 16) | (c & ~0x07E0);
    }

    FORCE_INLINE_FUNCTION static uint16_t compactRgb565(uint32_t c)
    {
        return ((c >> 16) & 0x07E0) | (c & ~0x07E0);
    }

    FORCE_INLINE_FUNCTION static uint16_t bilinearInterpolate565(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, uint8_t x, uint8_t y)
    {
        assert(x < 16 && y < 16);
        uint32_t a00 = expandRgb565(c00);
        uint32_t a10 = expandRgb565(c10);
        uint32_t a01 = expandRgb565(c01);
        uint32_t a11 = expandRgb565(c11);

        uint8_t xy = (x * y) >> 3;
        return compactRgb565((a00 * (32 - 2 * y - 2 * x + xy) + a10 * (2 * x - xy) + a01 * (2 * y - xy) + a11 * xy) >> 5);
    }

    FORCE_INLINE_FUNCTION static uint8_t bilinearInterpolate8(uint8_t c00, uint8_t c10, uint8_t c01, uint8_t c11, uint8_t x, uint8_t y)
    {
        assert(x < 16 && y < 16);
        const uint16_t xy11 = x * y;
        const uint16_t xy10 = 16 * x - xy11;
        const uint16_t xy01 = 16 * y - xy11;
        const uint16_t xy00 = 256 - (xy11 + xy10 + xy01);

        return (c00 * xy00 + c10 * xy10 + c01 * xy01 + c11 * xy11) >> 8;
    }

    FORCE_INLINE_FUNCTION static uint32_t bilinearInterpolate888(uint32_t c00, uint32_t c10, uint8_t x)
    {
        assert(x < 16);
        const uint16_t xy00 = 16 - x;

        return ((((c00 & 0xFF00FF) * xy00 + (c10 & 0xFF00FF) * x) >> 4) & 0xFF00FF) | ((((c00 & 0x00FF00) * xy00 + (c10 & 0x00FF00) * x) >> 4) & 0x00FF00);
    }

    FORCE_INLINE_FUNCTION static uint32_t bilinearInterpolate888(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11, uint8_t x, uint8_t y)
    {
        assert(x < 16 && y < 16);
        const uint16_t xy11 = x * y;
        const uint16_t xy10 = 16 * x - xy11;
        const uint16_t xy01 = 16 * y - xy11;
        const uint16_t xy00 = 256 - (xy11 + xy10 + xy01);

        return ((((c00 & 0xFF00FF) * xy00 + (c10 & 0xFF00FF) * xy10 + (c01 & 0xFF00FF) * xy01 + (c11 & 0xFF00FF) * xy11) >> 8) & 0xFF00FF) | ((((c00 & 0x00FF00) * xy00 + (c10 & 0x00FF00) * xy10 + (c01 & 0x00FF00) * xy01 + (c11 & 0x00FF00) * xy11) >> 8) & 0x00FF00);
    }

    FORCE_INLINE_FUNCTION static uint32_t div255_888(uint32_t val, uint8_t factor)
    {
        return div255rb((val & 0xFF00FF) * factor) | div255g((val & 0x00FF00) * factor);
    }

    FORCE_INLINE_FUNCTION static uint32_t div255_888_FFcheck(uint32_t val, uint8_t factor)
    {
        return factor < 0xFF ? div255_888(val, factor) : val;
    }

    FORCE_INLINE_FUNCTION static void copy888(const uint8_t* const rgb888, uint8_t* const destBits)
    {
        destBits[0] = rgb888[0];
        destBits[1] = rgb888[1];
        destBits[2] = rgb888[2];
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888(const uint8_t r, const uint8_t g, const uint8_t b, uint8_t* const destBits, const uint8_t alpha, const uint8_t ialpha)
    {
        destBits[0] = div255(b * alpha + destBits[0] * ialpha);
        destBits[1] = div255(g * alpha + destBits[1] * ialpha);
        destBits[2] = div255(r * alpha + destBits[2] * ialpha);
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888(const uint8_t* const rgb888, uint8_t* const destBits, const uint8_t alpha, const uint8_t ialpha)
    {
        alphaBlend888(rgb888[2], rgb888[1], rgb888[0], destBits, alpha, ialpha);
    }

    FORCE_INLINE_FUNCTION static uint8_t cap_byte_value(int value)
    {
        return (value > 255) ? (255) : (value >= 0 ? value : 0);
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888_premul(const uint8_t redFg, const uint8_t greenFg, const uint8_t blueFg, const uint8_t alpha, const uint8_t alphaFg, uint8_t* const destBits)
    {
        if (alphaFg == 255)
        {
            destBits[0] = blueFg;
            destBits[1] = greenFg;
            destBits[2] = redFg;
            destBits[3] = 255;
        }
        else if (alphaFg > 0)
        {
            const uint8_t alphaBg = destBits[3];
            const uint8_t alphaMult = LCD::div255(alphaFg * alphaBg);
            const uint8_t alphaOut = alphaFg + alphaBg - alphaMult;

            const uint8_t blueBg = destBits[0];
            destBits[0] = cap_byte_value((blueFg * alpha + blueBg * (alphaBg - alphaMult)) / alphaOut);
            const uint8_t greenBg = destBits[1];
            destBits[1] = cap_byte_value((greenFg * alpha + greenBg * (alphaBg - alphaMult)) / alphaOut);
            const uint8_t redBg = destBits[2];
            destBits[2] = cap_byte_value((redFg * alpha + redBg * (alphaBg - alphaMult)) / alphaOut);
            destBits[3] = alphaOut;
        }
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888_premul(const uint8_t* const rgb888, const uint8_t alpha, const uint8_t alphaFg, uint8_t* const destBits)
    {
        alphaBlend888_premul(rgb888[2], rgb888[1], rgb888[0], alpha, alphaFg, destBits);
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888(const uint8_t redFg, const uint8_t greenFg, const uint8_t blueFg, const uint8_t alphaFg, uint8_t* const destBits)
    {
        if (alphaFg == 255)
        {
            destBits[0] = blueFg;
            destBits[1] = greenFg;
            destBits[2] = redFg;
            destBits[3] = 0xFF;
        }
        else if (alphaFg > 0)
        {
            const uint8_t alphaBg = destBits[3];
            const uint8_t alphaMult = LCD::div255(alphaFg * alphaBg);
            const uint8_t alphaOut = alphaFg + alphaBg - alphaMult;

            const uint8_t blueBg = destBits[0];
            destBits[0] = (blueFg * alphaFg + blueBg * (alphaBg - alphaMult)) / alphaOut;
            const uint8_t greenBg = destBits[1];
            destBits[1] = (greenFg * alphaFg + greenBg * (alphaBg - alphaMult)) / alphaOut;
            const uint8_t redBg = destBits[2];
            destBits[2] = (redFg * alphaFg + redBg * (alphaBg - alphaMult)) / alphaOut;
            destBits[3] = alphaOut;
        }
    }

    FORCE_INLINE_FUNCTION static void alphaBlend888(const uint8_t* const rgb888, const uint8_t alphaFg, uint8_t* const destBits)
    {
        alphaBlend888(rgb888[2], rgb888[1], rgb888[0], alphaFg, destBits);
    }

    class DrawTextureMapScanLineBase16 : public DrawTextureMapScanLineBase
    {
    protected:
        FORCE_INLINE_FUNCTION bool overrunCheckNearestNeighbor(uint16_t*& destBits, int& pixelsToDraw, fixed16_16& U, fixed16_16& V, fixed16_16 deltaU, fixed16_16 deltaV, const int16_t maxWidth, const int16_t maxHeight);
        FORCE_INLINE_FUNCTION bool overrunCheckBilinearInterpolation(uint16_t*& destBits, int& pixelsToDraw, fixed16_16& U, fixed16_16& V, fixed16_16 deltaU, fixed16_16 deltaV, const int16_t maxWidth, const int16_t maxHeight);
    };

    class TextureMapper_16BPP_L8_RGB888_Opaque_NearestNeighbor_GA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t alpha);
    };

    class TextureMapper_16BPP_L8_RGB888_Opaque_NearestNeighbor_NoGA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt);
    };

    class TextureMapper_16BPP_L8_RGB888_Opaque_BilinearInterpolation_GA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
        void writePixelOnEdge(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
    };

    class TextureMapper_16BPP_L8_RGB888_Opaque_BilinearInterpolation_NoGA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
        void writePixelOnEdge(uint16_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
    };

    class TextureMapper_16BPP_L8_ARGB8888_NonOpaque_NearestNeighbor_GA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t alpha);
    };

    class TextureMapper_16BPP_L8_ARGB8888_NonOpaque_NearestNeighbor_NoGA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt);
    };

    class TextureMapper_16BPP_L8_ARGB8888_NonOpaque_BilinearInterpolation_GA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
        void writePixelOnEdge(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
    };

    class TextureMapper_16BPP_L8_ARGB8888_NonOpaque_BilinearInterpolation_NoGA : public DrawTextureMapScanLineBase16
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
        void writePixelOnEdge(uint16_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
    };

    class DrawTextureMapScanLineBase32 : public DrawTextureMapScanLineBase
    {
    protected:
        FORCE_INLINE_FUNCTION bool overrunCheckNearestNeighbor(uint32_t*& destBits, int& pixelsToDraw, fixed16_16& U, fixed16_16& V, fixed16_16 deltaU, fixed16_16 deltaV, const int16_t maxWidth, const int16_t maxHeight);
        FORCE_INLINE_FUNCTION bool overrunCheckBilinearInterpolation(uint32_t*& destBits, int& pixelsToDraw, fixed16_16& U, fixed16_16& V, fixed16_16 deltaU, fixed16_16 deltaV, const int16_t maxWidth, const int16_t maxHeight);
    };

    class TextureMapper_32BPP_L8_RGB888_Opaque_NearestNeighbor_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t alpha);
    };

    class TextureMapper_32BPP_L8_RGB888_Opaque_NearestNeighbor_NoGA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt);
    };

    class TextureMapper_32BPP_L8_RGB888_Opaque_BilinearInterpolation_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
        void writePixelOnEdge(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
    };

    class TextureMapper_32BPP_L8_RGB888_Opaque_BilinearInterpolation_NoGA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
        void writePixelOnEdge(uint32_t* const destBits, const uint8_t* const textureBits8, const uint8_t* const palette8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
    };

    class TextureMapper_32BPP_L8_ARGB8888_NonOpaque_NearestNeighbor_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t alpha);
    };

    class TextureMapper_32BPP_L8_ARGB8888_NonOpaque_BilinearInterpolation_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
        void writePixelOnEdge(uint32_t* const destBits, const uint8_t* const textureBits8, const uint32_t* const palette32, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
    };

    class TextureMapper_32BPP_RGB888_Opaque_NearestNeighbor_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t alpha);
    };

    class TextureMapper_32BPP_RGB888_Opaque_NearestNeighbor_NoGA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int UInt, const int VInt);
    };

    class TextureMapper_32BPP_RGB888_Opaque_BilinearInterpolation_GA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
        void writePixelOnEdge(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac, const uint8_t alpha);
    };

    class TextureMapper_32BPP_RGB888_Opaque_BilinearInterpolation_NoGA : public DrawTextureMapScanLineBase32
    {
    public:
        virtual void drawTextureMapScanLineSubdivisions(int subdivisions, const int widthModLength, int pixelsToDraw, const int affineLength, float oneOverZRight, float UOverZRight, float VOverZRight, fixed16_16 U, fixed16_16 V, fixed16_16 deltaU, fixed16_16 deltaV, float ULeft, float VLeft, float URight, float VRight, float ZRight, const DrawingSurface& dest, const int destX, const int destY, const TextureSurface& texture, uint8_t alpha, const float dOneOverZdXAff, const float dUOverZdXAff, const float dVOverZdXAff);

    private:
        FORCE_INLINE_FUNCTION void writePixel(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
        void writePixelOnEdge(uint32_t* const destBits, const uint8_t* const textureBits8, const int16_t bitmapWidth, const int16_t bitmapHeight, const int UInt, const int VInt, const uint8_t UFrac, const uint8_t VFrac);
    };
};

} // namespace touchgfx
#endif // LCDGPU2D_AXI_HPP
