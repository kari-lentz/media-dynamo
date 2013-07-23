#ifndef ASSET_H
#define ASSET_H

#include <sstream>
#include <string>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "app-fault.h"

using namespace std;

typedef struct
{
    cairo_t* cr;
    cairo_surface_t* surface;
    int width;
    int height;
} scratch_pad_t;

class asset_t
{
protected:
    double alpha_;
    double r_;
    double g_;
    double b_;
    int begin_ms_;
    int end_ms_;
    int x_;
    int y_;
    int z_;
    int width_;
    int height_;

    cairo_surface_t* surface_;
    cairo_t* cr_;

public:

    asset_t();

    asset_t(double alpha, double r, double g, double b, int begin_ms, int end_ms, int x, int y, int z, int width, int height);

    ~asset_t();

    bool is_visible(int media_ms);
    virtual void render(cairo_t* cr);
};

class text_asset_t:public asset_t
{
private:
    string markup_;

public:
    text_asset_t(scratch_pad_t* scratch_pad, const char* markup, double alpha, double r, double g, double a, int begin_ms, int end_ms, int x, int y, int z, int width, int height);
    ~text_asset_t();
};

class bitmap_asset_t:public asset_t
{
private:
    string path_;

public:

    bitmap_asset_t(scratch_pad_t* scratch_pad, const char* path, double alpha, double r, double g, double a, int begin_ms, int end_ms, int x, int y, int z, int width, int height);
    ~bitmap_asset_t();
};

#endif
