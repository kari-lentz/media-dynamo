#ifndef CAIRO_F_H_
#define CAIRO_F_H_

#include <map>
#include <string>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

typedef enum
{
    eWeightNormal = 0,
    eWeightBold = 1
} font_weight_t;

typedef enum
{
    eSlantNormal = 0,
    eSlantItalic = 1
} font_slant_t;

using namespace std;

class cairo_f
{
public:

    static map<font_slant_t, string> font_slants_;
    static map<font_weight_t, string> font_weights_;
    static void register_all();
    static string get_font_slant(font_slant_t font_slant);
    static string get_font_weight(font_weight_t font_weight);

    cairo_f();
    virtual void render(cairo_t* cr)=0;
};

class move_to_f:public cairo_f
{
private:
    double x_;
    double y_;

public:

    move_to_f(double x, double y);
    void render(cairo_t* cr);
};

class translate_f:public cairo_f
{
private:
    double x_;
    double y_;

public:

    translate_f(double x, double y);
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

class show_text_f:public cairo_f
{
private:
    string text_;
    string font_;

public:
    show_text_f(const char* text, const char* font_face, font_weight_t weight, font_slant_t font_slant, int font_size);
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
