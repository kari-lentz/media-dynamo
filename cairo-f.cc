#include "cairo-f.h"

cairo_f::cairo_f(){}

push_f::push_f():cairo_f()
{
}

void push_f::render(cairo_t* cr,dom_context_stack_t& dom_context_stack)
{
    dom_context_t* dc = !dom_context_stack.empty() ? new dom_context_t( dom_context_stack.top()) : new dom_context_t(cr);
    dom_context_stack.push(dc);
    cairo_save(cr);
}

pop_f::pop_f():cairo_f()
{
}

void pop_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        delete dom_context_stack.top();
        dom_context_stack.pop();
    }

    cairo_restore(cr);
};

move_to_f::move_to_f(double x, double y):cairo_f(), x_(x), y_(y)
{
}

void move_to_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    cairo_move_to(cr, x_, y_);
}

translate_f::translate_f(double x, double y):cairo_f(), x_(x), y_(y)
{
}

void translate_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    cairo_translate(cr, x_, y_);
}

set_source_rgba_f::set_source_rgba_f(double r, double g, double b,double a):cairo_f(),r_(r), g_(g), b_(b), a_(a)
{
}

void set_source_rgba_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    cairo_set_source_rgba(cr, r_, g_, b_, a_);
}

set_font_family_f::set_font_family_f(const char* name):cairo_f(), name_(name)
{
}

void set_font_family_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_font_family(name_.c_str());
    }
}

set_font_weight_f::set_font_weight_f(font_weight_t weight):cairo_f(), weight_(weight)
{
}

void set_font_weight_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_font_weight(weight_);
    }
};

set_font_style_f::set_font_style_f(font_style_t style):cairo_f(), style_(style)
{
}

void set_font_style_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_font_style(style_);
    }
}

set_font_size_f::set_font_size_f(int size):cairo_f(), size_(size)
{
}

void set_font_size_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_font_size(size_);
    }
};

set_layout_width_f::set_layout_width_f(int width):cairo_f(), width_(width)
{
}

void set_layout_width_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_layout_width(width_);
    }
}

set_layout_alignment_f::set_layout_alignment_f(alignment_t align):cairo_f(), align_(align)
{
}

void set_layout_alignment_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_layout_alignment(align_);
    }
};

set_layout_indent_f::set_layout_indent_f(int indent):cairo_f(), indent_(indent)
{
}

void set_layout_indent_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->set_layout_indent(indent_);
    }
}

show_text_f::show_text_f( const char* text):cairo_f(), text_(text)
{
}

void show_text_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    if( !dom_context_stack.empty() )
    {
        dom_context_stack.top()->show_text(cr, text_.c_str());
    }
};

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

void show_png_f::render(cairo_t* cr, dom_context_stack_t& dom_context_stack)
{
    cairo_set_source_surface(cr, surface_, x_, y_);

    cairo_pattern_t* nothing = cairo_pattern_create_rgba(0, 0, 0, alpha_);
    cairo_mask (cr, nothing);

    cairo_fill(cr);
};
