#include "dom-context.h"

map<font_style_t, PangoStyle> dom_context_t::font_styles_;
map<font_weight_t, PangoWeight> dom_context_t::font_weights_;
map<alignment_t, PangoAlignment> dom_context_t::layout_alignments_;

dom_context_t::dom_context_t(cairo_t* cr)
{
    desc_ = pango_font_description_new();
    layout_ = pango_cairo_create_layout(cr);
}

dom_context_t::dom_context_t(dom_context_t* dom_context)
{
    desc_ = pango_font_description_copy(dom_context->desc_);
    layout_ = pango_layout_copy( dom_context->layout_ );
}

dom_context_t::~dom_context_t()
{
    if( desc_ ) pango_font_description_free( desc_ );
    if( layout_  ) g_object_unref( layout_ );
}

void dom_context_t::set_font_family(const char* name)
{
    pango_font_description_set_family(desc_, name);
}

void dom_context_t::set_font_style(font_style_t style)
{
    pango_font_description_set_style(desc_, get_font_style(style));
}

void dom_context_t::set_font_weight(font_weight_t weight)
{
    pango_font_description_set_weight(desc_, get_font_weight(weight));
}

void dom_context_t::set_font_size(int size)
{
    pango_font_description_set_size(desc_, size * PANGO_SCALE);
}

void dom_context_t::set_layout_width( int width )
{
    pango_layout_set_width( layout_, width * PANGO_SCALE );
}

void dom_context_t::set_layout_alignment(alignment_t align)
{
    pango_layout_set_alignment( layout_, get_layout_alignment(align) );
}

void dom_context_t::set_layout_indent(int indent)
{
    pango_layout_set_indent( layout_, indent * PANGO_SCALE );
}

void dom_context_t::show_text(cairo_t* cr, const char* text)
{
    pango_layout_set_text(layout_, text, -1);
    if( pango_layout_get_font_description(layout_) != desc_ )
    {
        pango_layout_set_font_description(layout_, desc_);
    }
    pango_cairo_update_layout(cr, layout_);
    pango_cairo_show_layout(cr, layout_);
}

