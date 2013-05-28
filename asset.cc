#include "asset.h"

asset_t::asset_t():alpha_(0),r_(0),g_(0),b_(0),begin_ms_(0), end_ms_(0), x_(0),y_(0), z_(0), width_(0),height_(0), surface_(NULL), cr_(NULL)
{
}

asset_t::asset_t(double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height): alpha_(alpha), r_(r), g_(g), b_(b), begin_ms_(begin_ms), end_ms_(end_ms), x_(x),y_(y), z_(z), width_(width),height_(height),surface_(NULL), cr_(NULL)
{
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);

    if( !surface_ )
    {
        stringstream ss;
        ss << "could not create surface" << endl;
        throw app_fault( "" );
    }

    cr_ = cairo_create( surface_ );

    if( !cr_ )
    {
        stringstream ss;
        ss << "could not create render context for surface" << endl;
        throw app_fault( "" );
    }

    cairo_set_source_rgb(cr_, r_, g_, b_);
}

asset_t::~asset_t()
{
    if( cr_ )
    {
        cairo_destroy( cr_ );
        cr_ = NULL;
    }

    if( surface_ )
    {
        cairo_surface_destroy( surface_ );
        surface_ = NULL;
    }
}

void asset_t::render(cairo_t* cr)
{
    cairo_set_source_surface(cr, surface_, x_, y_);

    cairo_pattern_t* nothing = cairo_pattern_create_rgba(0, 0, 0, alpha_);
    cairo_mask (cr, nothing);
    cairo_fill(cr);

    cairo_pattern_destroy(nothing);
}

text_asset_t::text_asset_t(const char* markup, double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height):asset_t(alpha, r, g, b, begin_ms, end_ms, x, y, z, width, height),markup_(markup), layout_(NULL), desc_(NULL)
{
    desc_ = pango_font_description_new();
    layout_ = pango_cairo_create_layout(cr_);

    pango_font_description_set_family(desc_, "arial");
    pango_font_description_set_style(desc_, PANGO_STYLE_NORMAL);
    pango_font_description_set_weight(desc_, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(desc_, 12 * PANGO_SCALE);
    pango_layout_set_width( layout_, width * PANGO_SCALE );
    pango_layout_set_alignment( layout_, PANGO_ALIGN_CENTER );

    int indent = 0;
    pango_layout_set_indent( layout_, indent * PANGO_SCALE );

    pango_layout_set_markup(layout_, markup_.c_str(), -1);
    pango_cairo_update_layout(cr_, layout_);

    pango_cairo_show_layout(cr_, layout_);
}

text_asset_t::~text_asset_t()
{
    if( desc_ )
    {
        pango_font_description_free( desc_ );
        desc_ = NULL;
    }

    if( layout_  )
    {
        g_object_unref( layout_ );
        layout_ = NULL;
    }
};

bitmap_asset_t::bitmap_asset_t(const char* path, double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height):asset_t(alpha, r, g, b, begin_ms, end_ms, x, y, z, width, height),path_(path)
{
    bitmap_surface_ = cairo_image_surface_create_from_png(path_.c_str());
    bitmap_cr_ = cairo_create(bitmap_surface_);

    if( !bitmap_surface_ )
    {
        stringstream ss;
        ss << "can't create surface for " << path;
        throw app_fault(ss.str().c_str());
    }

    cairo_set_source_surface(cr_, bitmap_surface_, 0, 0);
    cairo_fill(cr_);
}

bitmap_asset_t::~bitmap_asset_t()
{
    if( bitmap_cr_ )
    {
        cairo_destroy( bitmap_cr_ );
        bitmap_cr_ = NULL;
    }

    if( bitmap_surface_ )
    {
        cairo_surface_destroy( bitmap_surface_ );
        bitmap_surface_ = NULL;
    }
};
