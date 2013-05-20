#ifndef CAIRO_F_H_
#define CAIRO_F_H_

#include <cairo/cairo.h>

class cairo_f
{
public:
    cairo_f();
    virtual void render(cairo_t* cr)=0;
};

class move_to_f:public cairo_f
{
private:
    int x_;
    int y_;

public:

    move_to_f(int x, int y);
    void render(cairo_t* cr);
};

class set_source_rgba_f:public cairo_f
{
private:
    double r_;
    double g_;
    double b_;
    double a_;

public:

    set_source_rgba_f(double r, double g, double b,double a);
    void render(cairo_t* cr);
};

class set_font_size_f:public cairo_f
{
private:
    int size_;

public:

    set_font_size_f(int size);
    void render(cairo_t* cr);
};

class select_font_face_f:public cairo_f
{
private:
    string font_face_;
    cairo_font_slant_t slant_;
    cairo_font_weight_t weight_;

public:
    select_font_face_f(const char* font_face, cairo_font_slant_t slant, cairo_font_weight_t weight);
    void render(cairo_t* cr);
};

class show_text_f:public cairo_f
{
private:
    string text_;

public:

    show_text_f(const char* text);
    void render(cairo_t* cr);
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
    void render(cairo_t* cr);
};

#endif
