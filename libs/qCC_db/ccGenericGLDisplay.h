//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#ifndef CC_GENERIC_GL_DISPLAY
#define CC_GENERIC_GL_DISPLAY

//Always first
#include "ccIncludeGL.h"

//Local
#include "qCC_db.h"
#include "ccSerializableObject.h"
#include "ccGLMatrix.h"
#include "ccMaterial.h"

//Qt
#include <QImage>
#include <QString>
#include <QFont>

//CCLib
#include <CCGeom.h>

class QWidget;

//! Standard parameters for GL displays/viewports
class QCC_DB_LIB_API ccViewportParameters : public ccSerializableObject
{
public:
	//! Default constructor
	ccViewportParameters();

	//! Copy constructor
	ccViewportParameters(const ccViewportParameters& params);

	//inherited from ccSerializableObject
	virtual bool isSerializable() const { return true; }
	virtual bool toFile(QFile& out) const;
	virtual bool fromFile(QFile& in, short dataVersion, int flags);

	//! Current pixel size (in 'current unit'/pixel)
	/** This scale is valid eveywhere in ortho. mode 
		or at the focal distance in perspective mode.
		Warning: doesn't take current zoom into account!
	**/
	float pixelSize;

	//! Current zoom
	float zoom;

	//! Visualization matrix (rotation only)
	ccGLMatrixd viewMat;

	//! Point size
	float defaultPointSize;
	//! Line width
	float defaultLineWidth;

	//! Perspective view state
	bool perspectiveView;
	//! Whether view is centered on displayed scene (true) or on the user eye (false)
	/** Always true for ortho. mode.
	**/
	bool objectCenteredView;

	//! Theoretical perspective 'zNear' relative position
	double zNearCoef;
	//! Actual perspective 'zNear' value
	double zNear;
	//! Actual perspective 'zFar' value
	double zFar;
	
	//! Rotation pivot point (for object-centered view modes)
	CCVector3d pivotPoint;
	
	//! Camera center (for perspective mode)
	CCVector3d cameraCenter;

	//! Camera F.O.V. (field of view - for perspective mode only)
	float fov;
	//! Camera aspect ratio (perspective mode only)
	float perspectiveAspectRatio;

	//! 3D view aspect ratio (ortho mode only)
	/** AR = width / height
	**/
	float orthoAspectRatio;

};

//! OpenGL camera parameters
struct ccGLCameraParameters
{
	ccGLCameraParameters()
		: perspective(false)
		, fov_deg(0)
		, pixelSize(0)
	{
	   memset(viewport, 0, 4 * sizeof(int));
	}

	//! Projects a 3D point in 2D (+ normalized 'z' coordinate)
	inline bool project(const CCVector3d& input3D, CCVector3d& output2D) const { return ccGL::Project<double, double>(input3D, modelViewMat.data(), projectionMat.data(), viewport, output2D); }
	//! Projects a 3D point in 2D (+ normalized 'z' coordinate)
	inline bool project(const CCVector3& input3D, CCVector3d& output2D) const { return ccGL::Project<PointCoordinateType, double>(input3D, modelViewMat.data(), projectionMat.data(), viewport, output2D); }

	//! Unprojects a 2D point (+ normalized 'z' coordinate) in 3D
	inline bool unproject(const CCVector3d& input2D, CCVector3d& output3D) const { return ccGL::Unproject<double, double>(input2D, modelViewMat.data(), projectionMat.data(), viewport, output3D); }
	//! Unprojects a 2D point (+ normalized 'z' coordinate) in 3D
	inline bool unproject(const CCVector3& input2D, CCVector3d& output3D) const { return ccGL::Unproject<PointCoordinateType, double>(input2D, modelViewMat.data(), projectionMat.data(), viewport, output3D); }

	//! Model view matrix (GL_MODELVIEW)
	ccGLMatrixd modelViewMat;
	//! Projection matrix (GL_PROJECTION)
	ccGLMatrixd projectionMat;
	//! Viewport (GL_VIEWPORT)
	int viewport[4];
	//! Perspective mode
	bool perspective;
	//! F.O.V. (in degrees) - perspective mode only
	float fov_deg;
	//! Pixel size (i.e. zoom) - non perspective mode only
	float pixelSize;
};

//! Generic interface for GL displays
class ccGenericGLDisplay
{
public:

	//! Returns the screen size
	virtual QSize getScreenSize() const = 0;

	//! Redraws display immediately
	virtual void redraw(bool only2D = false, bool resetLOD = true) = 0;

	//! Flags display as 'to be refreshed'
	/** See ccGenericGLDisplay::refresh.
	**/
	virtual void toBeRefreshed() = 0;

	//! Redraws display only if flagged as 'to be refreshed'
	/** See ccGenericGLDisplay::toBeRefreshed. Flag is turned
		to false after a call to this method.
		\param only2D whether to redraw everything (false) or only the 2D layer (true)
	**/
	virtual void refresh(bool only2D = false) = 0;

	//! Invalidates current viewport setup
	/** On next redraw, viewport information will be recomputed.
	**/
	virtual void invalidateViewport() = 0;

	//! Returns the texture ID corresponding to an image
	virtual unsigned getTextureID(const QImage& image) = 0;
	
	//! Returns the texture ID corresponding to a material
	virtual unsigned getTextureID( ccMaterial::CShared mtl) = 0;

	//! Release texture from context
	virtual void releaseTexture(unsigned texID) = 0;

	//! Returns defaul text display font
	/** Warning: already takes rendering zoom into account!
	**/
	virtual QFont getTextDisplayFont() const = 0;

	//! Returns defaul label display font
	/** Warning: already takes rendering zoom into account!
	**/
	virtual QFont getLabelDisplayFont() const = 0;

	//! Text alignment
	enum TextAlign { ALIGN_HLEFT	= 1,
					 ALIGN_HMIDDLE	= 2,
					 ALIGN_HRIGHT	= 4,
					 ALIGN_VTOP		= 8,
					 ALIGN_VMIDDLE	= 16,
					 ALIGN_VBOTTOM	= 32,
					 ALIGN_DEFAULT	= 1 | 8};

	//! Displays a string at a given 2D position
	/** This method should be called solely during 2D pass rendering.
		The coordinates are expressed relatively to the current viewport (y = 0 at the top!).
		\param text string
		\param x horizontal position of string origin
		\param y vertical position of string origin
		\param align alignment position flags
		\param bkgAlpha background transparency (0 by default)
		\param rgbColor text color (optional)
		\param font optional font (otherwise default one will be used)
	**/
	virtual void displayText(	QString text,
								int x,
								int y,
								unsigned char align = ALIGN_DEFAULT,
								float bkgAlpha = 0,
								const unsigned char* rgbColor = 0,
								const QFont* font = 0) = 0;

	//! Displays a string at a given 3D position
	/** This method should be called solely during 3D pass rendering (see paintGL).
		\param str string
		\param pos3D 3D position of string origin
		\param rgbColor color (optional: if let to 0, default text rendering color is used)
		\param font font (optional)
	**/
	virtual void display3DLabel(const QString& str,
								const CCVector3& pos3D,
								const unsigned char* rgbColor = 0,
								const QFont& font=QFont()) = 0;

	//! Returns the current OpenGL camera parameters
	virtual void getGLCameraParameters(ccGLCameraParameters& params) = 0;

	//! Returns viewport parameters (zoom, etc.)
	virtual const ccViewportParameters& getViewportParameters() const = 0;

	//! Setups a (projective) camera
	/** \param cameraMatrix orientation/position matrix of the camera
		\param fov_deg vertical field of view (in degrees). Optional (ignored if 0).
		\param ar aspect ratio (width/height)
		\param viewerBasedPerspective whether the perspective view should be object-centered (false) or camera-centered (true)
		\param bubbleViewMode set whether bubble-view mode should be enabled or not (in which case viewerBasedPerspective is forced by default)
	**/
	virtual void setupProjectiveViewport(	const ccGLMatrixd& cameraMatrix,
											float fov_deg = 0.0f,
											float ar = 1.0f,
											bool viewerBasedPerspective = true,
											bool bubbleViewMode = false) = 0;

	//! Returns this window as a proper Qt widget
	virtual QWidget* asWidget() { return 0; }

	//! Returns the set of OpenGL functions
	virtual ccQOpenGLFunctions* functions() = 0;

};

#endif //CC_GENERIC_GL_DISPLAY
