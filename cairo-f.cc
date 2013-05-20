#include "ring-buffer-video.h"
#include "app-fault.h"

#include "cairo-f.h"

cairo_f::cairo_f()
{}

move_to_f::move_to_f(int x, int y):cairo_f(), x_(x), y_(y)
{
}

void move_to_f::render(cairo_t* cr)
{
    cairo_move_to(cr, x_, y_);
}

set_source_rgba_f::set_source_rgba_f(double r, double g, double b,double a):cairo_f(),r_(r), g_(g), b_(b), a_(a)
{
}

void set_source_rgba_f::render(cairo_t* cr)
{
    cairo_set_source_rgba(cr, r_, g_, b_, a_);
}

set_font_size_f::set_font_size_f(int size):cairo_f(),size_(size)
{
}

void set_font_size_f::render(cairo_t* cr)
{
    cairo_set_font_size(cr, size_);
}

select_font_face_f::select_font_face_f(const char* font_face, cairo_font_slant_t slant, cairo_font_weight_t weight):cairo_f(),font_face_(font_face), slant_(slant), weight_(weight)
{
}

void select_font_face_f::render(cairo_t* cr)
{
    cairo_select_font_face(cr, font_face_.c_str(), slant_, weight_);
}

show_text_f::show_text_f(const char* text):cairo_f(),text_(text)
{
}

void show_text_f::render(cairo_t* cr)
{
    cairo_show_text(cr, text_.c_str());
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
