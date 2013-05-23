#ifndef CAIRO_F_H_
#define CAIRO_F_H_

#include <string>
#include <sstream>
#include <stack>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "dom-context.h"
#include "app-fault.h"

using namespace std;

class dom_context_stack_t:public stack<dom_context_t*>
{
public:

    dom_context_stack_t(cairo_t* cr)
    {
        dom_context_t* dc = new dom_context_t(cr);
        push(dc);
    }

    ~dom_context_stack_t()
    {
        while( !empty() )
        {
            delete top();
            pop();
        }
    }

};

class cairo_f
{
public:

    cairo_f();
    virtual void render(cairo_t* cr, dom_context_stack_t& dom_context_stack)=0;
};

class push_f:public cairo_f
{

public:

    push_f();
    void render(cairo_t* cr,dom_context_stack_t& dom_context_stack);
};

class pop_f:public cairo_f
{

public:

    pop_f();
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class move_to_f:public cairo_f
{
private:
    double x_, y_;

public:
    move_to_f(double x, double y);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};


class translate_f:public cairo_f
{
private:
    double x_, y_;

public:

    translate_f(double x, double y);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_source_rgba_f:public cairo_f
{
private:

    double r_,g_,b_,a_;

public:
    set_source_rgba_f(double r, double g, double b,double a);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_font_family_f:public cairo_f
{
private:
    string name_;

public:
    set_font_family_f(const char* name);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_font_weight_f:public cairo_f
{
private:
    font_weight_t weight_;

public:

    set_font_weight_f(font_weight_t weight);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_font_style_f:public cairo_f
{
private:
    font_style_t style_;

public:

    set_font_style_f(font_style_t style);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_font_size_f:public cairo_f
{
private:
    int size_;

public:

    set_font_size_f(int size);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_layout_width_f:public cairo_f
{
private:
    int width_;

public:

    set_layout_width_f(int width);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_layout_alignment_f:public cairo_f
{
private:
    alignment_t align_;

public:

    set_layout_alignment_f(alignment_t align);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class set_layout_indent_f:public cairo_f
{
private:
    int indent_;

public:

    set_layout_indent_f(int indent);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class show_text_f:public cairo_f
{
private:
    string text_;

public:

    show_text_f( const char* text);
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

class show_png_f:public cairo_f
{
private:
    cairo_surface_t* surface_;
    double x_;
    double y_;
    double alpha_;

public:

    show_png_f(const char* path, double x, double y, double alpha);
    ~show_png_f();
    void render(cairo_t* cr, dom_context_stack_t& dom_context_stack);
};

#endif
