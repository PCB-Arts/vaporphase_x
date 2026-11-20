#ifndef STM32GPU2DHAL_HPP
#define STM32GPU2DHAL_HPP

#include <touchgfx/hal/HAL.hpp>

#include <nema_cmdlist.h>

namespace touchgfx
{
class HALGPU2D : public touchgfx::HAL
{
public:
    /**
     * @fn STM32GPU2DHAL::STM32GPU2DHAL(touchgfx::DMA_Interface& dma, touchgfx::LCD& display, touchgfx::TouchController& tc, uint16_t width, uint16_t height) : touchgfx::HAL(dma, display, tc, width, height)
     *
     * @brief Constructor.
     *
     *        Constructor. Initializes members.
     *
     * @param [in,out] dma     Reference to DMA interface.
     * @param [in,out] display Reference to LCD interface.
     * @param [in,out] tc      Reference to Touch Controller driver.
     * @param width            Width of the display.
     * @param height           Height of the display.
     */
    HALGPU2D(touchgfx::DMA_Interface& dma, touchgfx::LCD& display, touchgfx::TouchController& tc, uint16_t width, uint16_t height);

    /** 
     * This function is responsible for initializing the HALGPU2D and the entire framework. 
     *
     * @param  clSize  Size of the GPU2D command list in bytes. Suggested value is 8192.
     *
     * @note The command list size has a big impact on performance. 8192 is a good value in most cases,
     *       so go with that if unsure. Recommended values are 4-8KB. Due to the nature of command lists, 
     *       assigning a larger buffer size might actually cause a decrease in performance.
     */
    void initialize(int clSize);
    
    /** 
     * This function is responsible for initializing the HALGPU2D and the entire framework. 
     * The default implementation will assign a GPU2D command list buffer of 8192 bytes.
     *
     * @see void initialize(int clSize);
     */
    void initialize() 
    { 
        initialize(8192); 
    }

    /**
     * @fn virtual uint16_t* HAL::lockFrameBuffer();
     *
     * @brief Waits for the frame buffer to become available for use.
     *
     *        Waits for the frame buffer to become available for use (i.e. not used by DMA
     *        transfers).
     *
     * @note Function blocks until frame buffer is available. Client code MUST call
     *       unlockFrameBuffer() when frame buffer operation has completed.
     *
     * @return A pointer to the beginning of the currently used frame buffer.
     */
    virtual uint16_t* lockFrameBuffer();

    /**
     * @fn virtual void HAL::unlockFrameBuffer();
     *
     * @brief Unlocks the frame buffer (MUST be called exactly once for each call to
     *        lockFrameBuffer()).
     *
     *        Unlocks the frame buffer (MUST be called exactly once for each call to
     *        lockFrameBuffer()).
     */
    virtual void unlockFrameBuffer();

    /**
     * @fn void bindExecBuffer();
     *
     * @brief Binds a new GPU2D Command List.
     */
    void bindExecBuffer();

    /**
     * Gets client framebuffer.
     *
     * @return The address of the framebuffer currently used by the framework to draw in.
     */
    uint16_t* getClientFrameBuffer()
    {
        return HAL::getClientFrameBuffer();
    }

    /**
     * @fn void submitExecBuffer();
     *
     * @brief Unbinds and submits the GPU2D Command List immediately.
     */
    void submitExecBuffer();

    /**
     * @fn void submitExecBufferOnEndframe();
     *
     * @brief Unbinds and submits the GPU2D Command List when frame ends.
     */
    void submitExecBufferOnEndframe()
    {
        submitOnEndFrame = true;
    }
    
    /**
     * @fn void clearFrameBuffer();
     *
     * @brief Clears the framebuffer.
     */
    void clearFrameBuffer();

    /**
     * @fn HALGPU2D* getInstance();
     *
     * @brief Get an instance of the HALGPU2D.
     */
    static HALGPU2D* getInstance()
    {
        assert(gpu2dHAL && "HALGPU2D not initialized!");
        return gpu2dHAL;
    }

    /**
     * Blits a 2D source-array to the framebuffer performing alpha-blending as specified.
     *
     * @param  pSrc                 The source-array pointer (points to first value to copy)
     * @param  pClut                The CLUT pointer (points to CLUT header data which include
     *                              the type and size of this CLUT followed by colors data)
     * @param  x                    The destination x coordinate on the framebuffer.
     * @param  y                    The destination y coordinate on the framebuffer.
     * @param  width                The width desired area of the source 2D array.
     * @param  height               The height of desired area of the source 2D array.
     * @param  srcWidth             The distance (in elements) from first value of first line, to
     *                              first value of second line (the source 2D array width)
     * @param  alpha                The alpha value to use for blending (255 = solid, no blending)
     * @param  hasTransparentPixels If true, this data copy contains transparent pixels and
     *                              require hardware support for that to be enabled.
     * @param  dstWidth             The distance (in elements) from first value of first line, to
     *                              first value of second line (the destination 2D array width)
     * @param  srcFormat            The source buffer color format (default is the framebuffer
     *                              format)
     * @param  dstFormat            The destination buffer color format (default is the
     *                              framebuffer format)
     *
     * @note Alpha=255 is assumed "solid" and shall be used if HAL does not support
     *       BLIT_OP_COPY_WITH_ALPHA.
     */
    void dma2dBlitCopy(const uint16_t* pSrc, const uint8_t* pClut, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t srcWidth, uint8_t alpha, bool hasTransparentPixels, uint16_t dstWidth, touchgfx::Bitmap::BitmapFormat srcFormat, touchgfx::Bitmap::BitmapFormat dstFormat);

protected:
    /**
     * @fn virtual bool HAL::beginFrame();
     *
     * @brief Called when beginning to rendering a frame.
     *
     *        Called when beginning to rendering a frame.
     *
     * @return true if rendering can begin, false otherwise.
     */
    virtual bool beginFrame();

    /**
     * @fn virtual void HAL::endFrame();
     *
     * @brief Called when a rendering pass is completed.
     *
     *        Called when a rendering pass is completed.
     */
    virtual void endFrame();

private:
    static HALGPU2D* gpu2dHAL;              // Singleton HAL instance
    nema_cmdlist_t   execBuf;              // Command list buffer for GPU2D
    bool dma2dActive;
    bool submitOnEndFrame;
};

} // namespace touchgfx

#endif // STM32GPU2DHAL_HPP
