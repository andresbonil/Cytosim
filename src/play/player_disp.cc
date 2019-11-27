// Cytosim was created by Francois Nedelec. Copyright 2007-2017 EMBL.

#include "sim.h"
#include "meca.h"
#include "modulo.h"
#include "simul_prop.h"
#include "display1.h"
#include "display2.h"
#include "display3.h"
#include "saveimage.h"
#include <unistd.h>
#include <cstdlib>
#include "glut.h"

extern void helpKeys(std::ostream&);

extern Modulo const* modulo;

//------------------------------------------------------------------------------
#pragma mark -

void Player::setStyle(const int style)
{
    if ( mDisplay )
    {
        //restore the previous OpenGL state
        glPopAttrib();
        delete(mDisplay);
        mDisplay = nullptr;
    }
    
    //save the current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    switch ( style )
    {
        default:
        case 1: mDisplay = new Display1(&DP);  break;
        case 2: mDisplay = new Display2(&DP);  break;
        case 3: mDisplay = new Display3(&DP);  break;
    }
    DP.style = style;

    //initialize Views associated with opened GLUT windows:
    for ( size_t n = 1; n < glApp::views.size(); ++n )
    {
        View & view = glApp::views[n];
        if ( view.window() > 0 )
        {
            //std::clog << "initializing GLUT window " << n << std::endl;
            //glutSetWindow(view.window());
            view.initGL();
            glViewport(0, 0, view.width(), view.height());
        }
    }
}


/**
 Build a message containing the label and the time.
 For an interactive window, it also adds 'live' or the frame index,
 and the force generated by the mouse.
 */
std::string Player::buildLabel() const
{
    std::ostringstream oss;
    oss.precision(3);

    oss << std::setw(8) << std::fixed << simul.time() << "s";
    
    //display the force exerted by the mouse-controled Single:
    Single const* sh = thread.handle();
    if ( sh && sh->attached() )
        oss << "\nHandle: " << sh->force().norm() << "pN";

    if ( thread.alive() && goLive )
    {
        oss << "\nLive";
        //display ratio number-of-time-step / frame
        if ( PP.period > 1 )
            oss << " " << PP.period;
    }
    else
    {
        oss << "\nFrame " << thread.currentFrame();
    }

    return oss.str();
}


/**
 This information is displayed in the top corner of the window.
 Calling simul.report() maked sure that the message is identical to what
 would be printed by the command 'report'.
 */
std::string Player::buildReport(std::string arg) const
{
    if ( ! arg.empty() )
    {
        Glossary glos;
        // separate options:
        std::string::size_type pos = arg.find(' ');
        if ( pos != std::string::npos )
        {
            glos.read_string(arg.substr(pos+1).c_str(), 2);
            arg = arg.substr(0, pos);
        }
        try
        {
            std::stringstream ss;
            simul.report(ss, arg, glos);
            std::string res = ss.str();
            if ( res.size() > 1  &&  res.at(0) == '\n' )
                return res.substr(1);
            return res;
        }
        catch ( Exception & e )
        {
            return e.what();
        }
    }
    return "";
}

/**
 This text is normally displayed in the center of the window
 */
std::string Player::buildMemo(int type) const
{
    std::ostringstream oss;
    switch ( type )
    {
        case 0: return "";
        case 1: return "Please, visit www.cytosim.org";
        case 2: helpKeys(oss); return oss.str();
        case 3: glApp::help(oss);  return oss.str();
        case 4: writePlayParameters(oss, true); return oss.str();
        case 5: writeDisplayParameters(oss, true); return oss.str();
    }
    return "";
}

//------------------------------------------------------------------------------
#pragma mark - Display

void Player::autoTrack(FiberSet const& fibers, View& view)
{
    Vector G(0, 0, 0);
    Vector D(0, 0, 0);
    real vec[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    
    if ( view.track_fibers & 1 )
    {
        Vector M, P;
        FiberSet::infoPosition(fibers.collect(), M, G, P);
        view.move_shift(Vector3(G));
        //std::clog << "auto center: " << G << std::endl;
    }
    
    if ( view.track_fibers & 2 )
    {
        // align with mean nematic direction
        FiberSet::infoNematic(fibers.collect(), vec);
        view.align_with(Vector3(vec));
        //view.rotation.setFromMatrix3(vec);
        //view.rotation.conjugate();
        //std::clog << "auto rotate: " << Vector3(vec) << std::endl;
    }

    if ( view.track_fibers & 4 )
    {
        real sum = 0;
        real avg[3] = { 0 };
        real mom[9] = { 0 };
        FiberSet::infoComponents(fibers.collect(), sum, avg, mom, vec);
        // get rotation from matrix:
        view.rotation.setFromMatrix3(vec);
        // inverse rotation:
        view.rotation.conjugate();
        //std::clog << "auto quat: " << view.rotation << std::endl;
    }
}


/**
 Adjust to see the biggest Space in simul
 */
void Player::autoScale(SpaceSet const& spaces, View& view)
{
    real rad = 0;
    for ( Space const* spc = spaces.first(); spc; spc=spc->next() )
        rad = std::max(rad, spc->max_extension());
    if ( rad > 0 )
    {
        //std::clog << "auto_scale " << rad << '\n';
        view.view_size = GLfloat(2*rad);
        view.zoom_in(0.933033);
        --view.auto_scale;
    }
}


void Player::prepareDisplay(View& view, int mag)
{    
    //gle::gleReportErrors(stderr, "before prepareDisplay");
    
    //----------------- automatic adjustment of viewing area:

    if ( view.auto_scale > 0 )
        autoScale(simul.spaces, view);
    
    //----------------- auto-track:
    
    if ( view.track_fibers )
        autoTrack(simul.fibers, view);
    
    //----------------- texts:
    
    view.setLabel(buildLabel());
    view.setMessage(buildReport(PP.report));
    
    //----------------- set pixel size and unit-size:
    /*
     if DP.point_value is set, line-width and point-size were specified in 'real' units,
     and otherwise, they were specified in pixels.
     */

    GLfloat pix = view.pixelSize();
    //std::clog << " pixel size = " << pix << '\n';

    if ( DP.point_value > 0 )
        mDisplay->setPixelFactors(pix/mag, mag*DP.point_value/pix);
    else
        mDisplay->setPixelFactors(pix/mag, mag);

    gle::gleReportErrors(stderr, "before prepareDisplay");

    try {
        mDisplay->setStencil(view.stencil && ( DIM == 3 ));
        mDisplay->prepareForDisplay(simul, dproperties);
        //std::clog << " dproperties.size() = " << dproperties.size() << '\n';
    }
    catch(Exception & e) {
        std::cerr<<"Error in prepareDisplay: " << e.what() << '\n';
    }
}

//------------------------------------------------------------------------------
void Player::displayCytosim()
{
    // clear pixels:
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    try {
        // draw:
        if ( modulo && DP.tile )
            mDisplay->displayTiled(simul, DP.tile);
        else
            mDisplay->display(simul);

#if DRAW_MECA_LINKS
        if ( DP.draw_links )
        {
            glPushAttrib(GL_LIGHTING_BIT);
            glDisable(GL_LIGHTING);
            glLineWidth(4);
            glPointSize(8);
            glEnable(GL_LINE_STIPPLE);
            const_cast<Simul&>(simul).drawLinks();
            glDisable(GL_LINE_STIPPLE);
            glPopAttrib();
            gle::gleReportErrors(stderr, "Simul::drawLinks()");
        }
#endif
    }
    catch(Exception & e) {
        std::cerr<<"Error in display: " << e.what() << std::endl;
    }
}


void Player::readDisplayString(View& view, std::string const& str)
{
    //std::clog << "readDisplayString " << str << std::endl;
    try
    {
        Glossary glos(str);
        DP.read(glos);
        const int W = view.width(), H = view.height();
        view.read(glos);
        // window size cannot be changed:
        view.window_size[0] = W;
        view.window_size[1] = H;
    }
    catch( Exception & e )
    {
        std::cerr << "Error while reading simul:display: " << e.what();
    }
}


/**
 This display the full Scene
 */
void Player::displayScene(View& view, int mag)
{
    if ( simul.prop->display_fresh )
    {
        readDisplayString(view, simul.prop->display);
        simul.prop->display_fresh = false;
    }
    //thread.debug("display");
    prepareDisplay(view, mag);
    view.openDisplay();
    displayCytosim();
    view.closeDisplay();
}

//------------------------------------------------------------------------------
#pragma mark - Export Image

/**
 Export image from the current OpenGL back buffer,
 in the format specified by 'PlayProp::image_format',
 in the folder specified in `PlayProp::image_dir`.
 The name of the file is formed by concatenating 'root' and 'indx'.
 */
int Player::saveView(const char* root, unsigned indx, int verbose) const
{
    char cwd[1024] = { 0 };
    char name[1024];
    char const* format = PP.image_format.c_str();
    snprintf(name, sizeof(name), "%s%04i.%s", root, indx, format);
    if ( PP.image_dir.length() )
    {
        if ( getcwd(cwd, sizeof(cwd)) )
            chdir(PP.image_dir.c_str());
    }
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int err = SaveImage::saveImage(name, format, vp, PP.downsample);
    if ( err == 0 && verbose > 0 )
    {
        int W = vp[2] / PP.downsample;
        int H = vp[3] / PP.downsample;
        if ( verbose > 1 )
            printf("\r saved %ix%i snapshot %s    ", W, H, name);
        else
            printf(" saved %ix%i snapshot %s\n", W, H, name);
        fflush(stdout);
    }
    if ( cwd[0] )
        chdir(cwd);
    return err;
}


//------------------------------------------------------------------------------

void displayMagnified(int mag, void * arg)
{
    static_cast<Player*>(arg)->displayCytosim();
    gle::gleReportErrors(stderr, "in displayMagnified");
}


/**
 save an image where the resolution is magnified by a factor `mag`.
 This requires access to the simulation world.
 */
int Player::saveViewMagnified(const int mag, const char* name, const char* format, const int downsample)
{
    if ( !SaveImage::supported(format) )
    {
        std::cerr << "Error unsupported image format `" << format << "'\n";
        return -1;
    }
    
    View & view = glApp::currentView();
    const int W = view.width(), H = view.height();
    
    thread.lock();
    
    //std::clog << "saveMagnifiedImage " << W << "x" << H << " mag=" << mag << std::endl;

    prepareDisplay(view, mag);
    
    view.openDisplay();
    int err = SaveImage::saveMagnifiedImage(mag, name, format, W, H, displayMagnified, this, downsample);
    if ( err )
    {
        err = SaveImage::saveCompositeImage(mag, name, format, W, H, view.pixelSize(), displayMagnified, this);
        if ( !err )
            printf("saved %ix%i snapshot %s\n", W, H, name);
    }
    else
        printf("saved %ix%i snapshot %s\n", mag*W/downsample, mag*H/downsample, name);
    view.closeDisplay();
    thread.unlock();
    return err;
}


/**
 save an image where the resolution is magnified by a factor `mag`.
 This requires access to the simulation world.
 */
int Player::saveViewMagnified(const int mag, const char* root, unsigned indx, const int downsample)
{
    char cwd[1024] = { 0 };
    char name[1024];
    char const* format = PP.image_format.c_str();
    snprintf(name, sizeof(name), "%s%04i.%s", root, indx, format);
    if ( PP.image_dir.length() )
    {
        if ( getcwd(cwd, sizeof(cwd)) )
            chdir(PP.image_dir.c_str());
    }
    int err = saveViewMagnified(mag, name, format, downsample);
    if ( cwd[0] )
        chdir(cwd);
    glApp::postRedisplay();
    return err;
}

