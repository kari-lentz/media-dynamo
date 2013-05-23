#ifndef DOM_CONTEXT_H_
#define DOM_CONTEXT_H_

#include <map>
#include <cairo.h>
#include <pango/pangocairo.h>

using namespace std;

typedef enum
{
    eWeightNormal = 0,
    eWeightBold = 1
} font_weight_t;

typedef enum
{
    eStyleNormal = 0,
    eStyleItalic = 1,
    eStyleOblique = 2
} font_style_t;

typedef enum
{
    eAlignCenter = 0,
    eAlignLeft = 1,
    eAlignRight = 2,
} alignment_t;

class dom_context_t
{
private:

    static map<font_style_t, PangoStyle> font_styles_;
    static map<font_weight_t, PangoWeight> font_weights_;
    static map<alignment_t, PangoAlignment> layout_alignments_;

    PangoLayout* layout_;
    PangoFontDescription* desc_;

public:

    static void register_all()
    {
        font_styles_[ eStyleNormal ] = PANGO_STYLE_NORMAL;
        font_styles_[ eStyleItalic ] = PANGO_STYLE_ITALIC;
        font_styles_[ eStyleOblique ] = PANGO_STYLE_OBLIQUE;

        font_weights_[ eWeightNormal ] = PANGO_WEIGHT_NORMAL;
        font_weights_[ eWeightBold ] = PANGO_WEIGHT_BOLD;

        layout_alignments_[ eAlignCenter ] = PANGO_ALIGN_CENTER;
        layout_alignments_[ eAlignLeft ] = PANGO_ALIGN_LEFT;
        layout_alignments_[ eAlignRight ] = PANGO_ALIGN_RIGHT;
    }

    static PangoStyle get_font_style(font_style_t font_style)
    {
        map<font_style_t, PangoStyle>::iterator it = font_styles_.find( font_style );
        return (it != font_styles_.end()) ? (*it).second : PANGO_STYLE_NORMAL;
    }

    static PangoWeight get_font_weight(font_weight_t font_weight)
    {
        map<font_weight_t, PangoWeight>::iterator it = font_weights_.find( font_weight );
        return (it != font_weights_.end()) ? (*it).second : PANGO_WEIGHT_NORMAL;
    }

    static PangoAlignment get_layout_alignment(alignment_t align)
    {
        map<alignment_t, PangoAlignment>::iterator it = layout_alignments_.find( align );
        return (it != layout_alignments_.end()) ? (*it).second : PANGO_ALIGN_CENTER;
    }

    dom_context_t(cairo_t* cr);
    dom_context_t(dom_context_t* dom_context);
    ~dom_context_t();

    void set_font_family(const char* name);
    void set_font_style(font_style_t style);
    void set_font_weight(font_weight_t weight);
    void set_font_size(int size);
    void set_layout_width( int width );
    void set_layout_alignment(alignment_t align);
    void set_layout_indent(int indent);
    void show_text(cairo_t* cr, const char* text);
};

#endif
