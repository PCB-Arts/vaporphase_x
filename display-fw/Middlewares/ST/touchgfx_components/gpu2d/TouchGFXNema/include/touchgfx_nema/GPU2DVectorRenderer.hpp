#ifndef GPU2DVECTORRENDERER_HPP
#define GPU2DVECTORRENDERER_HPP

#include <touchgfx/hal/Types.hpp>
#include <touchgfx/hal/VectorRenderer.hpp>

namespace touchgfx
{
class GPU2DVectorRenderer : public VectorRenderer
{
public:
    /**
     * Start a new drawing (invalidated area) within the given canvas area.
     * The canvas area typically corresponds to the area of a Widget and is
     * given in absolute coordinates, whereas the invalidated area is given
     * in relative coordinates.
     * Implementations may lock the framebuffer here.
     *
     * @param  canvasAreaAbs        The canvas dimensions in absolute coordinates.
     * @param  invalidatedAreaRel   The area which should be updated in relative coordinates.
     */
    virtual void setup(const Rect& canvasAreaAbs, const Rect& invalidatedAreaRel);

    /**
     * Set the drawing mode for the VectorRenderer. The mode will be
     * used for all subsequent drawings until setMode is called again.
     *
     * The default mode is non-zero filling.
     *
     * @param mode The filling mode.
     */
    virtual void setMode(DrawMode mode);

    /**
     * Finalizes the current drawing. The implementation may block
     * here until the drawing is done.
     *
     * The implementation is required to unlock the framebuffer when
     * tearDown is called.
     */
    virtual void tearDown();

    /**
     * Draw a Path defined by the given array of primitives
     * (commands) and an array of the corresponding floating point
     * coordinates.
     *
     * @param cmds   An array of VectorPrimitives
     * @param nCmds The number of commands in cmds
     * @param points An array of coordinates for the commands
     * @param nPoints The number of values in the points arrays
     */
    virtual void drawPath(const uint8_t* cmds, uint32_t nCmds, const float* points, uint32_t nPoints, const float* bbox);

    /**
     * Set the width used for future drawings using the VECTOR_STROKE
     * drawing mode.
     *
     * @param w The width used for stroke drawing.
     */
    virtual void setStrokeWidth(float w);

    /**
     * Set a constant color used for future drawings.
     *
     * @param c An ARGB color value
     */
    virtual void setColor(colortype c);

    /**
     * Set the alpha value (opacity) used for future drawings.
     * The alpha is specified in the interval [0;255].
     *
     * @param a The alpha value.
     */
    virtual void setAlpha(uint8_t a);

    /**
     * Set a linear gradient used for future drawings.
     * The linear gradient is transformed using the current
     * transformation matrix.
     * Multiple colors (stops) can be specified for the gradient. The
     * stop positions are given in the range [0;1].
     *
     * @param x0            X-coordinate for gradient starting point
     * @param y0            Y-coordinate for gradient starting point
     * @param x1            X-coordinate for gradient end point
     * @param y1            Y-coordinate for gradient end point
     * @param stops         Number of stops
     * @param stopPositions Positions of the stops on the line
     * @param stopColors    Colors of the stops
     * @param width         Width of the box to fill
     * @param height        Height of the box to fill
     * @param solid         True if all colors in the gradient are solid
     * @param palette       The gradient palette
     */
    virtual void setLinearGradient(float x0, float y0, float x1, float y1,
                                   uint32_t stops,
                                   const float* stopPositions,
                                   const uint32_t* stopColors,
                                   float width,
                                   float height,
                                   bool solid,
                                   const uint32_t* palette);

    /**
     * Set a radial gradient used for future drawings.
     * The radial gradient is transformed using the current
     * transformation matrix.
     * Multiple colors (stops) can be specified for the gradient. The
     * stop positions are given in the range [0;1].
     *
     * @param cx            X-coordinate for the gradient center
     * @param cy            Y-coordinate for the gradient center
     * @param radius        Radius of the gradient
     * @param stops         Number of stops
     * @param stopPositions Positions of the stops on the line
     * @param stopColors    Colors of the stops
     */
    virtual void setRadialGradient(float cx, float cy, float radius,
                                   uint32_t stops,
                                   const float* stopPositions,
                                   const colortype* stopColors);

    /**
     * Sets the transformation matrix used for future drawings.
     * The transformation is reset when setup is called.
     *
     * @param m The transformation matrix
     *
     */
    virtual void setTransformationMatrix(const Matrix3x3& m);

    /**
     * Stroke meter limit is unsupported in GPU2D
     *
     * @param  miterLimit ignored
     */
    virtual void setStrokeMiterLimit(float miterLimit) {}

    /**
     * Stroke line join is unsupported in GPU2D
     *
     * @param  lineJoin ignored
     */
    virtual void setStrokeLineJoin(VGStrokeLineJoin lineJoin) {}

    /**
     * Line cap type is unsupported in GPU2D
     *
     * @param  lineCap ignored
     */
    virtual void setStrokeLineCap(VGStrokeLineCap lineCap) {}
};
}
#endif