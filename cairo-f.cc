#include "ring-buffer-video.h"
#include "app-fault.h"

#include "cairo-f.h"

map<font_slant_t, string> cairo_f::font_slants_;
map<font_weight_t, string> cairo_f::font_weights_;

void cairo_f::register_all()
{
    font_slants_[ eSlantNormal ] = "Normal";
    font_slants_[ eSlantItalic ] = "Italic";

    font_weights_[ eWeightNormal ] = "Normal";
    font_weights_[ eWeightBold ] = "Bold";
}

string cairo_f::get_font_slant(font_slant_t font_slant)
{
    return font_slants_[ font_slant ];
}

string cairo_f::get_font_weight(font_weight_t font_weight)
{
    return font_weights_[ font_weight ];
}

cairo_f::cairo_f()
{}

move_to_f::move_to_f(double x, double y):cairo_f(), x_(x), y_(y)
{
}

void move_to_f::render(cairo_t* cr)
{
    cairo_move_to(cr, x_, y_);
}

translate_f::translate_f(double x, double y):cairo_f(), x_(x), y_(y)
{
}

void translate_f::render(cairo_t* cr)
{
    cairo_translate(cr, x_, y_);
}

set_source_rgba_f::set_source_rgba_f(double r, double g, double b,double a):cairo_f(),r_(r), g_(g), b_(b), a_(a)
{
}

void set_source_rgba_f::render(cairo_t* cr)
{
    cairo_set_source_rgba(cr, r_, g_, b_, a_);
}

show_text_f::show_text_f(const char* text, const char* font_family, font_weight_t font_weight, font_slant_t font_slant, int font_size):cairo_f(),text_(text)
{
    stringstream ss;
    ss << font_family;

    if( font_slant != eSlantNormal )
    {
        ss << " " << get_font_slant( font_slant );
    }

    if( font_weight != eWeightNormal )
    {
        ss << " " << get_font_weight( font_weight );
    }

    ss << " " << font_size;

    font_ = ss.str();
}

void show_text_f::render(cairo_t* cr)
{
    PangoLayout *layout;
    PangoFontDescription *desc;

    layout = pango_cairo_create_layout(cr);

    pango_layout_set_text(layout, text_.c_str(), -1);
    desc = pango_font_description_from_string(font_.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);

}

show_png_f::show_png_f(const char* path, double x, double y, double alpha):cairo_f(),surface_(NULL), x_(x),y_(y), alpha_(alpha)
{
    surface_ = cairo_image_surface_create_from_png(path);

    if( !surface_ )
    {
        stringstream ss;
        ss << "can't create surface for " << path;
        throw app_fault(ss.str().c_str());
    }
}

show_png_f::~show_png_f()
{
    if(surface_) cairo_surface_destroy(surface_);
}

void show_png_f::render(cairo_t* cr)
{
    cairo_save(cr);
    cairo_set_source_surface(cr, surface_, x_, y_);

    cairo_pattern_t* nothing = cairo_pattern_create_rgba(0, 0, 0, alpha_);
    cairo_mask (cr, nothing);

    cairo_fill(cr);
    cairo_restore(cr);
}
