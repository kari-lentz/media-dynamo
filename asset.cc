#include "asset.h"

asset_t::asset_t():alpha_(0),r_(0),g_(0),b_(0),begin_ms_(0), end_ms_(0), x_(0),y_(0), z_(0), width_(0),height_(0), surface_(NULL), cr_(NULL)
{
}

asset_t::asset_t(double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height): alpha_(alpha), r_(r), g_(g), b_(b), begin_ms_(begin_ms), end_ms_(end_ms), x_(x),y_(y), z_(z), width_(width),height_(height),surface_(NULL), cr_(NULL)
{
    if(width == -1 || height == -1)
    {
        surface_ = NULL;
        cr_ = NULL;
    }
    else
    {
        surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);

        if( !surface_ )
        {
            stringstream ss;
            ss << "could not create surface" << endl;
            throw app_fault( ss.str().c_str() );
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

text_asset_t::text_asset_t(scratch_pad_t* scratch_pad, const char* markup, double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height):asset_t(alpha, r, g, b, begin_ms, end_ms, x, y, z, width, height),markup_(markup)
{
    bool done_p = false;

    while(!done_p)
    {
        cairo_surface_t* surface = surface_ ? surface_ : scratch_pad->surface;
        cairo_t* cr = cr_ ? cr_ : scratch_pad->cr;

        PangoFontDescription* desc = pango_font_description_new();
        PangoLayout* layout = pango_cairo_create_layout(cr);

        pango_font_description_set_family(desc, "arial");
        pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
        pango_font_description_set_size(desc, 12 * PANGO_SCALE);

        if(width >= 0)
        {
            pango_layout_set_width(layout, width * PANGO_SCALE);
        }
        else if(scratch_pad->width > x)
        {
            pango_layout_set_width(layout, (scratch_pad->width - x) * PANGO_SCALE);
        }

        if(height >= 0)
        {
            pango_layout_set_height(layout, height * PANGO_SCALE);
        }
        else if(scratch_pad->height > y)
        {
            pango_layout_set_height(layout, (scratch_pad->height - y) * PANGO_SCALE);
        }

        pango_layout_set_alignment( layout, PANGO_ALIGN_CENTER );

        int indent = 0;
        pango_layout_set_indent( layout, indent * PANGO_SCALE );

        pango_layout_set_markup(layout, markup_.c_str(), -1);
        pango_cairo_update_layout(cr, layout);

        pango_cairo_show_layout(cr, layout);

        if( !surface_ )
        {
            int width, height;
            pango_layout_get_size(layout,&width ,&height);

            if(width_ < 0 ) width_ = width / PANGO_SCALE;
            if(height_ < 0) height_ = height / PANGO_SCALE;

            surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
            cr_ = cairo_create( surface_ );
        }
        else
        {
            done_p = true;
        }

        pango_font_description_free(desc);
        g_object_unref(layout);
    }

}

text_asset_t::~text_asset_t()
{
};

bitmap_asset_t::bitmap_asset_t(scratch_pad_t* scratch_pad, const char* path, double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height):asset_t(alpha, r, g, b, begin_ms, end_ms, x, y, z, width, height),path_(path)
{
    bool done_p = false;

    while( !done_p )
    {
        cairo_surface_t* surface = surface_ ? surface_ : scratch_pad->surface;
        cairo_t* cr = cr_ ? cr_ : scratch_pad->cr;

        cairo_surface_t* bitmap_surface = cairo_image_surface_create_from_png(path_.c_str());

        if( !bitmap_surface )
        {
            stringstream ss;
            ss << "can't create surface for " << path;
            throw app_fault(ss.str().c_str());
        }

        cairo_set_source_surface(cr, bitmap_surface, 0, 0);
        cairo_fill(cr);

        if( !surface )
        {
            if(width_ < 0) width_ = cairo_image_surface_get_width(bitmap_surface);
            if(height_ < 0) height_ = cairo_image_surface_get_height(bitmap_surface);

            surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
            cr_ = cairo_create( surface_ );
        }
        else
        {
            done_p = true;
        }

        cairo_surface_destroy( bitmap_surface );
    }
}

bitmap_asset_t::~bitmap_asset_t()
{
};
