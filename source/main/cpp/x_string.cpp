#include "string_t/x_string.h"

#include "xbase/x_allocator.h"
#include "xbase/x_integer.h"
#include "xbase/x_memory.h"
#include "xbase/x_runes.h"
#include "xbase/x_printf.h"

namespace xcore
{
    //==============================================================================
    class xview
    {
    public:
        static inline uchar32* get_ptr_unsafe(string_t::view& str, s32 i) { return &str.m_data->m_runes.m_runes.m_utf32.m_str[i]; }
        static inline uchar32  get_char_unsafe(string_t::view const& str, s32 i) { return str.m_data->m_runes.m_runes.m_utf32.m_str[i]; }
        static inline uchar32  get_char_unsafe(string_t const& str, s32 i) { return str.m_data.m_runes.m_runes.m_utf32.m_str[i]; }

        static inline void set_char_unsafe(string_t& str, s32 i, uchar32 c) { str.m_data.m_runes.m_runes.m_utf32.m_str[i] = c; }
        static inline void set_char_unsafe(string_t::view& str, s32 i, uchar32 c) { str.m_data->m_runes.m_runes.m_utf32.m_str[i] = c; }

        static inline runes_t&       get_runes(string_t& str) { return str.m_data.m_runes; }
        static inline runes_t const& get_runes(string_t const& str) { return str.m_data.m_runes; }
        static inline runes_t const& get_runes(string_t::view const& str) { return str.m_data->m_runes; }

        static inline bool str_has_view(string_t const& str, string_t::view const& vw) { return (vw.m_data == &str.m_data); }

        static void resize(string_t& str, s32 new_size)
        {
            if (str.cap() < new_size)
            {
                runes_t nrunes = str.m_data.m_alloc->allocate(0, new_size, str.m_data.m_runes.m_type);
                copy(str.m_data.m_runes, nrunes);
                str.m_data.m_alloc->deallocate(str.m_data.m_runes);
                str.m_data.m_runes = nrunes;
            }
            else
            {
                // TODO: What about shrinking ?
            }
        }

        static bool narrow_view(string_t::view& v, s32 move)
        {
            if (v.size() > 0)
            {
                // if negative then narrow the left side
                if (move < 0)
                {
                    move = -move;
                    if (move <= v.m_view.size())
                    {
                        v.m_view.from += move;
                        return true;
                    }
                }
                else
                {
                    if (move <= v.m_view.size())
                    {
                        v.m_view.to -= move;
                        return true;
                    }
                }
            }
            return false;
        }

        static bool move_view(string_t::view& v, s32 move)
        {
            // Check if the move doesn't result in a negative @from
            s32 const from = v.m_view.from + move;
            if (from < 0)
                return false;

            // Check if the movement doesn't invalidate this view
            s32 const to = v.m_view.to + move;
            if (to > v.m_data->m_runes.size())
                return false;

            // Movement is ok, new view is valid
            v.m_view.from = from;
            v.m_view.to   = to;
            return true;
        }

        static string_t::view select_before(const string_t::view& str, const string_t::view& selection)
        {
            string_t::view v(selection);
            v.m_view.to   = v.m_view.from;
            v.m_view.from = 0;
            return v;
        }
        static string_t::view select_before_included(const string_t::view& str, const string_t::view& selection)
        {
            string_t::view v(selection);
            v.m_view.to   = v.m_view.to;
            v.m_view.from = 0;
            return v;
        }

        static string_t::view select_after(const string_t::view& str, const string_t::view& sel)
        {
            string_t::view v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.from;
            return v;
        }

        static string_t::view select_after_excluded(const string_t::view& str, const string_t::view& sel)
        {
            string_t::view v(sel);
            v.m_view.to   = str.m_view.to;
            v.m_view.from = sel.m_view.to;
            return v;
        }

        static void insert_newspace(runes_t& r, s32 pos, s32 len)
        {
            s32 src = r.size() - 1;
            s32 dst = src + len;
            while (src >= pos)
            {
                r.m_runes.m_utf32.m_str[dst--] = r.m_runes.m_utf32.m_str[src--];
            }
            r.m_runes.m_utf32.m_end += len;
            r.m_runes.m_utf32.m_end[0] = '\0';
        }

        static void remove_selspace(runes_t& r, s32 pos, s32 len)
        {
            s32       src = pos + len;
            s32       dst = pos;
            s32 const end = pos + len;
            while (src < r.size())
            {
                r.m_runes.m_utf32.m_str[dst++] = r.m_runes.m_utf32.m_str[src++];
            }
            r.m_runes.m_utf32.m_end -= len;
            r.m_runes.m_utf32.m_end[0] = '\0';
        }

        static void insert(string_t& str, string_t::view const& pos, string_t::view const& insert)
        {
            s32 dst = pos.m_view.from;

            string_t::range insert_range(dst, dst + insert.size());
            xview::adjust_active_views(str, xview::INSERTION, insert_range);

            xview::resize(str, str.size() + insert.size() + 1);
            insert_newspace(xview::get_runes(str), dst, insert.size());
            s32 src = 0;
            while (src < insert.size())
            {
                uchar32 const c = xview::get_char_unsafe(insert, src);
                xview::set_char_unsafe(str, dst, c);
                ++src;
                ++dst;
            }
        }

        static void remove(string_t& str, string_t::view const& selection)
        {
            if (selection.is_empty())
                return;
            if (xview::str_has_view(str, selection))
            {
                remove_selspace(str.m_data.m_runes, selection.m_view.from, selection.size());
                // TODO: Decision to shrink the allocated memory of m_runes ?

                string_t::range remove_range(selection.m_view.from, selection.m_view.to);
                xview::adjust_active_views(str, xview::REMOVAL, remove_range);
            }
        }

        static void find_remove(string_t& str, const string_t::view& _find)
        {
            string_t::view strvw = str.full();
            string_t::view sel   = find(strvw, _find);
            if (sel.is_empty() == false)
            {
                remove(str, sel);
            }
        }

        static void find_replace(string_t& str, const string_t::view& _find, const string_t::view& replace)
        {
            string_t::view strvw  = str.full();
            string_t::view remove = find(strvw, _find);
            if (remove.is_empty() == false)
            {
                s32 const remove_from = remove.m_view.from;
                s32 const remove_len  = remove.size();
                s32 const diff        = remove_len - replace.size();
                if (diff > 0)
                {
                    // The string to replace the selection with is smaller, so we have to remove
                    // some space from the string.
                    remove_selspace(xview::get_runes(str), remove_from, diff);
                    // TODO: Decision to shrink the allocated memory of m_runes ?

                    string_t::range remove_range(remove_from, remove_from + diff);
                    xview::adjust_active_views(str, xview::REMOVAL, remove_range);
                }
                else if (diff < 0)
                {
                    // The string to replace the selection with is longer, so we have to insert some
                    // space into the string.
                    xview::resize(str, str.size() + (-diff));
                    insert_newspace(xview::get_runes(str), remove_from, -diff);

                    string_t::range insert_range(remove_from, remove_from + -diff);
                    xview::adjust_active_views(str, xview::INSERTION, insert_range);
                }
                // Copy string 'remove' into the (now) same size selection space
                s32       src = 0;
                s32       dst = remove_from;
                s32 const end = remove_from + replace.size();
                while (src < replace.size())
                {
                    str.m_data.m_runes.m_runes.m_utf32.m_str[dst++] = replace[src++];
                }
            }
        }

        static void remove_any(string_t& str, const string_t::view& any)
        {
            // Remove any of the characters in @charset from @str
            s32       d   = 0;
            s32       i   = 0;
            s32 const end = str.size();
            s32       r   = -1;
            while (i < end)
            {
                uchar32 const c = xview::get_char_unsafe(str, i);
                if (contains(any, c))
                {
                    r = i;
                    i++;
                }
                else
                {
                    if (r >= 0)
                    { // This might not be the first character(s)/range removed, if not then the views already
                        // have been adjusted according to the previous range removal. So here we have to adjust
                        // this removal range by shifting it left with 'gap'.
                        s32 const      gap = i - d;
                        string_t::range removal_range(r, i);
                        xview::shift_range(removal_range, gap);
                        xview::adjust_active_views(str, xview::REMOVAL, removal_range);
                        r = -1;
                    }

                    if (i > d)
                    {
                        xview::set_char_unsafe(str, d, c);
                    }
                    i++;
                    d++;
                }
            }
            s32 const l = i - d;
            if (l > 0)
            {
                str.m_data.m_runes.m_runes.m_utf32.m_end -= l;
                str.m_data.m_runes.m_runes.m_utf32.m_end[0] = '\0';
            }
        }

        static string_t::view get_default() { return string_t::view(nullptr); }

        static const s32 NONE     = 0;
        static const s32 LEFT     = 0x0800; // binary(0000,1000,0000,0000);
        static const s32 RIGHT    = 0x0010; // binary(0000,0000,0001,0000);
        static const s32 INSIDE   = 0x0180; // binary(0000,0001,1000,0000);
        static const s32 MATCH    = 0x03C0; // binary(0000,0011,1100,0000);
        static const s32 OVERLAP  = 0x07E0; // binary(0000,0111,1110,0000);
        static const s32 ENVELOPE = 0x37EC; // binary(0011,0111,1110,1100);

        static s32 compare(string_t::range const& lhs, string_t::range const& rhs)
        {
            // Return where 'rhs' is in relation to 'lhs'
            // --------| lhs |--------[ rhs ]--------        RIGHT
            // --------[ rhs ]--------| lhs |--------        LEFT
            // --------[ rhs ]          lhs |--------        LEFT INSIDE
            // --------|     lhs      [ rhs ]--------        RIGHT INSIDE
            // --------|    [ rhs ]     lhs |--------        INSIDE
            // --------[     rhs  lhs       ]--------        MATCH
            // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP
            // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP
            // --------[    | lhs |     rhs ]--------        ENVELOPE
            // --------[ lhs |          rhs ]--------        LEFT ENVELOPE
            // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE
            if (lhs.to <= rhs.from)
                return RIGHT;
            else if (lhs.from >= rhs.to)
                return LEFT;
            else if (lhs.from == rhs.from && lhs.to > rhs.to)
                return LEFT | INSIDE;
            else if (lhs.from < rhs.from && lhs.to == rhs.to)
                return RIGHT | INSIDE;
            else if (lhs.from < rhs.from && lhs.to > rhs.to)
                return INSIDE;
            else if (lhs.from == rhs.from && lhs.to == rhs.to)
                return MATCH;
            else if (rhs.from < lhs.from && rhs.to < lhs.to)
                return LEFT | OVERLAP;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return RIGHT | OVERLAP;
            else if (lhs.from == rhs.from && lhs.to < rhs.to)
                return LEFT | ENVELOPE;
            else if (rhs.from < lhs.from && lhs.to == rhs.to)
                return RIGHT | ENVELOPE;
            else if (rhs.from < lhs.from && rhs.to > lhs.to)
                return ENVELOPE;
            return NONE;
        }
        static inline s32 compute_range_overlap(string_t::range const& lhs, string_t::range const& rhs)
        {
            if (rhs.from < lhs.from && rhs.to < lhs.to)
                return rhs.to - lhs.from;
            else if (rhs.from > lhs.from && rhs.to > lhs.to)
                return lhs.to - rhs.from;
            return 0;
        }
        static inline void shift_range(string_t::range& v, s32 distance)
        {
            v.from += distance;
            v.to += distance;
        }
        static inline void extend_range(string_t::range& v, s32 distance)
        {
            if (distance > 0)
            { // Extend right side
                v.to += distance;
            }
            else
            { // Extend left side
                v.from += distance;
            }
        }
        static inline void narrow_range(string_t::range& v, s32 distance)
        {
            if (distance > 0)
            { // Narrow right side
                v.to -= distance;
            }
            else
            { // Narrow left side
                v.from -= distance;
            }
        }
        static inline void invalidate_range(string_t::range& v)
        {
            v.from = 0;
            v.to   = 0;
        }

        // When the string is modified views can become invalid, here we try to keep them
        // 'correct' as much as we can. For example when we insert text into the string we
        // can correct all of the existing views.
        // Removal of text from a string may invalidate views that intersect with the range
        // of text that is removed.
        // Clear and Reset will invalidate all views on the string.
        static const s32 REMOVAL   = 0;
        static const s32 INSERTION = 1;
        static const s32 CLEARED   = 2;
        static const s32 RELEASED  = 3;
        static void      adjust_active_views(string_t::view* v, s32 op_code, string_t::range op_range)
        {
            switch (op_code)
            {
                case REMOVAL:
                {
                    // --------| lhs |--------[ rhs ]--------        RIGHT = NOTHING
                    // --------[ rhs ]--------| lhs |--------        LEFT = SHIFT LEFT rhs.size()
                    // --------|    [ rhs ]     lhs |--------        INSIDE = NARROW RIGHT by rhs.size()
                    // --------[ rhs ]          lhs |--------        LEFT INSIDE = NARROW RIGHT by rhs.size()
                    // --------|     lhs      [ rhs ]--------        RIGHT INSIDE = NARROW RIGHT by rhs.size()
                    // --------[     rhs  lhs       ]--------        MATCH = INVALIDATE
                    // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = NARROW LEFT by overlap & SHIFT LEFT by rhs.size()-overlap
                    // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP = NARROW RIGHT by overlap
                    // --------[    | lhs |     rhs ]--------        ENVELOPE = INVALIDATE
                    // --------[ lhs |          rhs ]--------        LEFT ENVELOPE = INVALIDATE
                    // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE = INVALIDATE
                    s32 const c = compare(v->m_view, op_range);
                    switch (c)
                    {
                        case LEFT: shift_range(v->m_view, -op_range.size()); break;
                        case INSIDE: narrow_range(v->m_view, op_range.size()); break;
                        case LEFT | INSIDE:
                        case RIGHT | INSIDE: extend_range(v->m_view, -op_range.size()); break;
                        case MATCH:
                        case ENVELOPE:
                        case LEFT | ENVELOPE:
                        case RIGHT | ENVELOPE: invalidate_range(v->m_view); break;
                        case LEFT | OVERLAP:
                            extend_range(v->m_view, -(compute_range_overlap(v->m_view, op_range)));
                            shift_range(v->m_view, -(op_range.size() - compute_range_overlap(v->m_view, op_range)));
                            break;
                        case RIGHT | OVERLAP: extend_range(v->m_view, compute_range_overlap(v->m_view, op_range)); break;
                        case RIGHT: break;
                    }
                }
                break;

                case INSERTION:
                {
                    // --------| lhs |--------[ rhs ]--------        RIGHT = DO NOTHING
                    // --------[ rhs ]--------| lhs |--------        LEFT = SHIFT RIGHT by rhs.size()
                    // --------[ rhs ]          lhs |--------        LEFT INSIDE = SHIFT RIGHT by rhs.size()
                    // --------|     lhs      [ rhs ]--------        RIGHT INSIDE = EXTEND RIGHT by rhs.size()
                    // --------|    [ rhs ]     lhs |--------        INSIDE = EXTEND RIGHT by rhs.size()
                    // --------[     rhs  lhs       ]--------        MATCH = SHIFT RIGHT by rhs.size()
                    // --------[ rhs    |    ]  lhs |--------        LEFT OVERLAP = SHIFT RIGHT by rhs.size()
                    // --------| lhs    [    |  rhs ]--------        RIGHT OVERLAP = EXTEND RIGHT by rhs.size()
                    // --------[    | lhs |     rhs ]--------        ENVELOPE = SHIFT RIGHT by rhs.size()
                    // --------[ lhs |          rhs ]--------        LEFT ENVELOPE = SHIFT RIGHT by rhs.size()
                    // --------[ rhs         |  lhs ]--------        RIGHT ENVELOPE = SHIFT RIGHT by rhs.size()
                    s32 const c = compare(v->m_view, op_range);
                    switch (c)
                    {
                        case MATCH:
                        case LEFT:
                        case ENVELOPE:
                        case (LEFT | ENVELOPE):
                        case (RIGHT | ENVELOPE):
                        case (LEFT | OVERLAP):
                        case (LEFT | INSIDE): shift_range(v->m_view, op_range.size()); break;
                        case INSIDE:
                        case (RIGHT | INSIDE):
                        case (RIGHT | OVERLAP): extend_range(v->m_view, op_range.size()); break;
                        case RIGHT: break;
                    }
                }
                break;

                case CLEARED:
                case RELEASED:
                {
                    invalidate_range(v->m_view);
                    v->m_data = nullptr;
                }
                break;
            }
        }

        static void adjust_active_views(string_t& str, s32 op_code, string_t::range op_range)
        {
            string_t::view* list = str.m_data.m_views;
            if (list != nullptr)
            {
                string_t::view* iter = list;
                do
                {
                    adjust_active_views(iter, op_code, op_range);
                    iter = iter->m_next;
                } while (iter != list);
            }
        }
    };

    //==============================================================================
    static s32 WriteCodeUnit(uchar32 cdpt, uchar8*& pDst)
    {
        if (cdpt <= 0x7F)
        {
            *pDst++ = (uchar8)(cdpt);
            return 1;
        }
        else if (cdpt <= 0x7FF)
        {
            *pDst++ = (uchar8)(0xC0 | ((cdpt >> 6) & 0x1F));
            *pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
            return 2;
        }
        else if (cdpt <= 0xFFFF)
        {
            *pDst++ = (uchar8)(0xE0 | ((cdpt >> 12) & 0x0F));
            *pDst++ = (uchar8)(0x80 | ((cdpt >> 6) & 0x3F));
            *pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
            return 3;
        }
        else if (cdpt <= 0x10FFFF)
        {
            *pDst++ = (uchar8)(0xF0 | ((cdpt >> 18) & 0x07));
            *pDst++ = (uchar8)(0x80 | ((cdpt >> 12) & 0x3F));
            *pDst++ = (uchar8)(0x80 | ((cdpt >> 6) & 0x3F));
            *pDst++ = (uchar8)(0x80 | (cdpt & 0x3F));
            return 4;
        }
        return 0;
    }

    static s32 WriteCodeUnit(uchar32 cdpt, uchar16*& pDst)
    {
        if (cdpt < 0x10000)
        {
            *pDst++ = (uchar16)cdpt;
            return 1;
        }
        else
        {
            *pDst++ = (uchar16)(0xD7C0 + (cdpt >> 10));
            *pDst++ = (uchar16)(0xDC00 + (cdpt & 0x3FF));
            return 2;
        }
    }

    bool ReadCodePoint(uchar8 const*& str, uchar8 const* pSrcEnd, uchar32& cdpt)
    {
        static const uchar32 REPLACEMENT_CHAR = '?';

        if (((unsigned char)*str) < 0x80)
        {
            cdpt = (unsigned char)*str++;
        }
        else if (((unsigned char)*str) < 0xC0)
        {
            ++str;
            cdpt = REPLACEMENT_CHAR;
            return false;
        }
        else if (((unsigned char)*str) < 0xE0)
        {
            cdpt = (((unsigned char)*str++) & 0x1F) << 6;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F);
        }
        else if (((unsigned char)*str) < 0xF0)
        {
            cdpt = (((unsigned char)*str++) & 0x0F) << 12;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F) << 6;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F);
        }
        else if (((unsigned char)*str) < 0xF8)
        {
            cdpt = (((unsigned char)*str++) & 0x07) << 18;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F) << 12;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F) << 6;
            if (((unsigned char)*str) < 0x80 || ((unsigned char)*str) >= 0xC0)
            {
                cdpt = REPLACEMENT_CHAR;
                return false;
            }
            cdpt += (((unsigned char)*str++) & 0x3F);
        }
        else
        {
            ++str;
            cdpt = REPLACEMENT_CHAR;
            return false;
        }
        return true;
    }

    bool ReadCodePoint(uchar16 const*& pSrc, uchar16 const* pSrcEnd, uchar32& cdpt)
    {
        static const uchar32 REPLACEMENT_CHAR = '?';

        if (*pSrc < 0xD800 || *pSrc >= 0xE000)
        {
            cdpt = *pSrc++;
            return true;
        }
        if (*pSrc >= 0xDC00)
        {
            ++pSrc;
            cdpt = REPLACEMENT_CHAR;
            return true;
        }
        else if ((pSrc + 1) < pSrcEnd)
        {
            uchar32 const res = 0x10000 + ((*pSrc++ - 0xD800) << 10);
            if (*pSrc >= 0xDC00 && *pSrc < 0xE000)
            {
                cdpt = res + (*pSrc++ - 0xDC00);
                return true;
            }
            cdpt = REPLACEMENT_CHAR;
        }
        return false;
    }

    static s32 ascii_nr_chars(uchar const* src, uchar const*& src_end)
    {
        s32 c = 0;
        if (src_end == nullptr)
        {
            while (*src != '\0')
            {
                src++;
                c++;
            }
            src_end = src;
        }
        else
        {
            c = (s32)(src_end - src);
        }
        return c;
    }

    static void ascii_to_utf32(uchar const* src, uchar const* src_end, uchar32*& dst, uchar32 const* dst_end)
    {
        while (src < src_end && dst < dst_end)
        {
            uchar32 c = *src++;
            *dst++    = c;
        }
        return;
    }

    static s32 utf8_nr_chars(uchar8 const* src, uchar8 const* src_end)
    {
        s32 len = 0;
        while (src < src_end)
        {
            uchar32 c;
            if (!ReadCodePoint(src, src_end, c))
            {
                return -len;
            }
            ++len;
        }
        return len;
    }

    static void utf8_to_utf32(uchar8 const* src, uchar8 const* src_end, uchar32* dst, uchar32 const* dst_end)
    {
        while (src < src_end && dst < dst_end)
        {
            uchar32 c;
            if (!ReadCodePoint(src, src_end, c))
            {
                return;
            }
            *dst++ = c;
        }
        return;
    }

    static s32 utf16_nr_chars(uchar16 const* src, uchar16 const* src_end)
    {
        s32 len = 0;
        while (src < src_end)
        {
            uchar32 c;
            if (!ReadCodePoint(src, src_end, c))
            {
                return -len;
            }
            ++len;
        }
        return len;
    }

    static void utf16_to_utf32(uchar16 const* src, uchar16 const* src_end, uchar32* dst, uchar32 const* dst_end)
    {
        while (src < src_end && dst < dst_end)
        {
            uchar32 c;
            if (!ReadCodePoint(src, src_end, c))
            {
                return;
            }
            *dst++ = c;
        }
        return;
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    string_t::view::view(string_t::data* d) : m_data(d), m_next(nullptr), m_prev(nullptr) {}

    string_t::view::view(const view& other) : m_data(other.m_data), m_view(other.m_view), m_next(nullptr), m_prev(nullptr) { add(); }

    string_t::view::~view() { rem(); }

    s32 string_t::view::size() const { return m_view.size(); }

    bool string_t::view::is_empty() const { return m_view.size() == 0; }

    string_t string_t::view::to_string() const
    {
        if (m_data->m_alloc != nullptr && !m_data->m_runes.is_empty())
        {
            runes_t dstrunes = m_data->m_alloc->allocate(0, m_view.size(), m_data->m_runes.m_type);
            runes_t srcrunes = m_data->m_runes;
            srcrunes.m_runes.m_utf32.m_str += m_view.from;
            copy(srcrunes, dstrunes);

            string_t str;
            str.m_data.m_alloc = m_data->m_alloc;
            str.m_data.m_runes = dstrunes;
            return str;
        }
        return string_t();
    }

    string_t::view string_t::view::operator()(s32 to)
    {
        to = xmin(0, m_view.size());
        string_t::view v(m_data);
        v.m_view.from = m_view.from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    string_t::view string_t::view::operator()(s32 from, s32 to)
    {
        xsort(from, to);
        to   = xmin(to, m_view.size());
        from = xmin(from, to);
        string_t::view v(m_data);
        v.m_view.from = m_view.from + from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    string_t::view string_t::view::operator()(s32 to) const
    {
        to = xmin(0, m_view.size());
        string_t::view v(m_data);
        v.m_view.from = m_view.from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    string_t::view string_t::view::operator()(s32 from, s32 to) const
    {
        xsort(from, to);
        to   = xmin(to, m_view.size());
        from = xmin(from, to);
        string_t::view v(m_data);
        v.m_view.from = m_view.from + from;
        v.m_view.to   = m_view.from + to;
        v.add();
        return v;
    }

    uchar32 string_t::view::operator[](s32 index) const
    {
        if (index < 0 || index >= m_view.size())
            return '\0';
        return m_data->m_runes.m_runes.m_utf32.m_str[m_view.from + index];
    }

    string_t::view& string_t::view::operator=(string_t::view const& other)
    {
        rem();
        m_data = other.m_data;
        m_view = other.m_view;
        add();
        return *this;
    }

    bool string_t::view::operator==(string_t::view const& other) const
    {
        if (m_view.size() == other.m_view.size())
        {
            uchar32 const* tsrc = m_data->m_runes.m_runes.m_utf32.m_str + m_view.from;
            uchar32 const* tend = m_data->m_runes.m_runes.m_utf32.m_str + m_view.to;
            uchar32 const* osrc = other.m_data->m_runes.m_runes.m_utf32.m_str + other.m_view.from;
            uchar32 const* oend = other.m_data->m_runes.m_runes.m_utf32.m_str + other.m_view.to;
            while (tsrc < tend)
            {
                if (*tsrc++ != *osrc++)
                    return false;
            }
            return true;
        }
        return false;
    }

    bool string_t::view::operator!=(string_t::view const& other) const
    {
        if (m_view.size() == other.m_view.size())
        {
            uchar32 const* tsrc = m_data->m_runes.m_runes.m_utf32.m_str + m_view.from;
            uchar32 const* tend = m_data->m_runes.m_runes.m_utf32.m_str + m_view.to;
            uchar32 const* osrc = other.m_data->m_runes.m_runes.m_utf32.m_str + other.m_view.from;
            uchar32 const* oend = other.m_data->m_runes.m_runes.m_utf32.m_str + other.m_view.to;
            while (tsrc < tend)
            {
                if (*tsrc++ != *osrc++)
                    return true;
            }
            return false;
        }
        return true;
    }

    void string_t::view::add()
    {
        if (m_data != nullptr)
        {
            string_t::view*& list = m_data->m_views;
            if (list == nullptr)
            {
                list   = this;
                m_next = this;
                m_prev = this;
            }
            else
            {
                string_t::view* prev = list->m_prev;
                string_t::view* next = list;
                prev->m_next        = this;
                next->m_prev        = this;
                m_next              = next;
                m_prev              = prev;
                list                = this;
            }
        }
    }

    void string_t::view::rem()
    {
        if (m_data != nullptr && m_data->m_views != nullptr)
        {
            string_t::view*& list = m_data->m_views;

            string_t::view* prev = m_prev;
            string_t::view* next = m_next;
            prev->m_next        = next;
            next->m_prev        = prev;

            if (list == this)
            {
                list = next;
                if (list == this)
                {
                    list = nullptr;
                }
            }
        }
        m_data = nullptr;
        m_next = nullptr;
        m_prev = nullptr;
    }

    void string_t::view::invalidate()
    {
        m_data      = nullptr;
        m_view.from = 0;
        m_view.to   = 0;
    }

    crunes_t string_t::view::get_runes() const
    {
        if (m_data->m_runes.is_empty())
        {
            crunes_t r(m_data->m_runes);
            r.m_runes.m_utf32.m_str += m_view.from;
            r.m_runes.m_utf32.m_end = r.m_runes.m_utf32.m_str + m_view.size();
            return r;
        }
        return crunes_t();
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    class runes_allocator : public runes_alloc_t
    {
    public:
        virtual runes_t allocate(s32 len, s32 cap, s32 type)
        {
            if (len > cap)
                cap = len;

            ASSERT(type == utf32::TYPE);
            u32 const    runesize = 4;
            utf32::prune str      = (utf32::prune)alloc_t::get_system()->allocate(cap * runesize, sizeof(void*));

            runes_t r;
            r.m_type                         = utf32::TYPE;
            r.m_runes.m_utf32.m_bos          = str;
            r.m_runes.m_utf32.m_str          = str;
            r.m_runes.m_utf32.m_end          = str + len;
            r.m_runes.m_utf32.m_eos          = str + cap - 1;
            r.m_runes.m_utf32.m_end[0]       = '\0';
            r.m_runes.m_utf32.m_end[cap - 1] = '\0';
            return r;
        }

        virtual void deallocate(runes_t& r)
        {
            if (r.m_runes.m_utf32.m_bos != nullptr)
            {
                alloc_t::get_system()->deallocate(r.m_runes.m_utf32.m_bos);
                r = runes_t();
            }
        }
    };

    static runes_allocator s_utf32_runes_allocator;
    runes_alloc_t*         string_t::s_allocator = &s_utf32_runes_allocator;

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    string_t::string_t(void) : m_data(s_allocator) {}

    string_t::string_t(runes_alloc_t* allocator) : m_data(allocator) {}

    string_t::string_t(runes_alloc_t* allocator, const char* str) : m_data(allocator)
    {
        const char* end = nullptr;
        s32 const   len = ascii_nr_chars(str, end) + 1;
        m_data.m_runes  = m_data.m_alloc->allocate(0, len, m_data.m_runes.m_type);
        ascii_to_utf32(str, end, m_data.m_runes.m_runes.m_utf32.m_end, m_data.m_runes.m_runes.m_utf32.m_eos);
    }

    string_t::string_t(const char* str) : m_data(s_allocator)
    {
        const char* end = nullptr;
        s32 const   len = ascii_nr_chars(str, end) + 1;
        m_data.m_runes  = m_data.m_alloc->allocate(0, len, m_data.m_runes.m_type);
        ascii_to_utf32(str, end, m_data.m_runes.m_runes.m_utf32.m_end, m_data.m_runes.m_runes.m_utf32.m_eos);
    }

    string_t::string_t(runes_alloc_t* _allocator, s32 _len) : m_data(_allocator) { m_data.m_runes = m_data.m_alloc->allocate(0, _len, utf32::TYPE); }

    string_t::string_t(const string_t& other) : m_data(other.m_data)
    {
        if (m_data.m_alloc != nullptr && !other.m_data.m_runes.is_empty())
        {
            m_data.m_runes = m_data.m_alloc->allocate(0, other.m_data.m_runes.size() + 1, m_data.m_runes.m_type);
            copy(other.m_data.m_runes, m_data.m_runes);
        }
    }

    string_t::string_t(const string_t::view& left, const string_t::view& right)
    {
        m_data.m_alloc = left.m_data->m_alloc;
        if (m_data.m_alloc != nullptr)
        {
            s32 cap        = left.size() + right.size() + 1;
            m_data.m_runes = m_data.m_alloc->allocate(0, cap, left.m_data->m_runes.m_type);
            concatenate(m_data.m_runes, left.get_runes(), right.get_runes(), m_data.m_alloc, 16);
        }
    }

    string_t::~string_t()
    {
        if (m_data.m_alloc != nullptr)
        {
            xview::adjust_active_views(*this, xview::CLEARED, string_t::range(0, 0));
            m_data.m_alloc->deallocate(m_data.m_runes);
        }
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    bool string_t::is_empty() const { return m_data.m_runes.size() == 0; }

    s32 string_t::size() const { return m_data.m_runes.size(); }

    s32 string_t::cap() const { return m_data.m_runes.cap(); }

    void string_t::clear()
    {
        m_data.m_runes.clear();
        xview::adjust_active_views(*this, xview::CLEARED, string_t::range(0, 0));
    }

    string_t::view string_t::full()
    {
        string_t::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = m_data.m_runes.size();
        v.add();
        return v;
    }

    string_t::view string_t::full() const
    {
        string_t::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = m_data.m_runes.size();
        v.add();
        return v;
    }

    string_t::view string_t::operator()(s32 to)
    {
        string_t::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = xmin(size(), xmax((s32)0, to));
        v.add();
        return v;
    }

    string_t::view string_t::operator()(s32 from, s32 to)
    {
        xsort(from, to);
        to   = xmin(to, size());
        from = xmin(from, to);
        string_t::view v(&m_data);
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add();
        return v;
    }

    string_t::view string_t::operator()(s32 to) const
    {
        string_t::view v(&m_data);
        v.m_view.from = 0;
        v.m_view.to   = xmin(size(), xmax((s32)0, to));
        v.add();
        return v;
    }

    string_t::view string_t::operator()(s32 from, s32 to) const
    {
        xsort(from, to);
        to   = xmin(to, size());
        from = xmin(from, to);
        string_t::view v(&m_data);
        v.m_view.from = from;
        v.m_view.to   = to;
        v.add();
        return v;
    }

    uchar32 string_t::operator[](s32 index) const
    {
        if (index >= m_data.m_runes.size())
            return '\0';
        return m_data.m_runes.m_runes.m_utf32.m_str[index];
    }

    string_t& string_t::operator=(const string_t& other)
    {
        if (this != &other)
        {
            release();
            clone(other.m_data.m_runes, other.m_data.m_alloc);
        }
        return *this;
    }

    string_t& string_t::operator=(const string_t::view& other)
    {
        if (&m_data != other.m_data)
        {
            release();
            clone(other.m_data->m_runes, other.m_data->m_alloc);
        }
        return *this;
    }

    bool string_t::operator==(const string_t& other) const
    {
        if (size() != other.size())
            return false;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(*this, i);
            uchar32 const rc = xview::get_char_unsafe(other, i);
            if (lc != rc)
                return false;
        }
        return true;
    }

    bool string_t::operator!=(const string_t& other) const
    {
        if (size() != other.size())
            return true;
        for (s32 i = 0; i < size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(*this, i);
            uchar32 const rc = xview::get_char_unsafe(other, i);
            if (lc != rc)
                return true;
        }
        return false;
    }

    void string_t::release()
    {
        if (m_data.m_alloc != nullptr)
        {
            xview::adjust_active_views(*this, xview::RELEASED, string_t::range(0, 0));
            m_data.m_alloc->deallocate(m_data.m_runes);
        }
    }

    void string_t::clone(runes_t const& str, runes_alloc_t* allocator)
    {
        m_data.m_alloc = allocator;
        m_data.m_runes = m_data.m_alloc->allocate(0, str.size() + 1, str.m_type);
        copy(str, m_data.m_runes);
    }

    //------------------------------------------------------------------------------
    string_t::view selectUntil(const string_t::view& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (c == find)
            {
                return str(0, i);
            }
        }
        return xview::get_default();
    }

    string_t::view selectUntil(const string_t::view& str, const string_t::view& find)
    {
        string_t::view v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return xview::select_before(str, v);
            }
            if (!xview::move_view(v, 1))
                break;
        }
        return xview::get_default();
    }

    string_t::view selectUntilLast(const string_t::view& str, uchar32 find)
    {
        for (s32 i = str.size() - 1; i >= 0; --i)
        {
            uchar32 const c = str[i];
            if (c == find)
            {
                return str(0, i);
            }
        }
        return xview::get_default();
    }

    string_t::view selectUntilLast(const string_t::view& str, const string_t::view& find)
    {
        string_t::view v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before view @v.
                return xview::select_before(str, v);
            }
            if (!xview::move_view(v, -1))
                break;
        }
        return xview::get_default();
    }

    string_t::view selectUntilIncluded(const string_t::view& str, const string_t::view& find)
    {
        string_t::view v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find and we need to return
                // a string view that exist before and includes view @v.
                return xview::select_before_included(str, v);
            }
            if (!xview::move_view(v, 1))
                break;
        }
        return xview::get_default();
    }

    string_t::view selectUntilEndExcludeSelection(const string_t::view& str, const string_t::view& selection) { return xview::select_after_excluded(str, selection); }

    string_t::view selectUntilEndIncludeSelection(const string_t::view& str, const string_t::view& selection) { return xview::select_after(str, selection); }

    bool isUpper(const string_t::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (is_lower(c))
                return false;
        }
        return true;
    }

    bool isLower(const string_t::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (is_upper(c))
                return false;
        }
        return true;
    }

    bool isCapitalized(const string_t::view& str)
    {
        s32 i = 0;
        while (i < str.size())
        {
            uchar32 c = '\0';
            while (i < str.size())
            {
                c = xview::get_char_unsafe(str, i);
                if (!is_space(c))
                    break;
                i += 1;
            }

            if (is_upper(c))
            {
                i += 1;
                while (i < str.size())
                {
                    c = xview::get_char_unsafe(str, i);
                    if (is_space(c))
                        break;
                    if (is_upper(c))
                        return false;
                    i += 1;
                }
            }
            else if (is_alpha(c))
            {
                return false;
            }
            i += 1;
        }
        return true;
    }

    bool isQuoted(const string_t::view& str) { return isQuoted(str, '"'); }

    bool isQuoted(const string_t::view& str, uchar32 inQuote) { return isDelimited(str, inQuote, inQuote); }

    bool isDelimited(const string_t::view& str, uchar32 inLeft, uchar32 inRight)
    {
        if (str.is_empty())
            return false;
        s32 const first = 0;
        s32 const last  = str.size() - 1;
        return str[first] == inLeft && str[last] == inRight;
    }

    uchar32 firstChar(const string_t::view& str)
    {
        s32 const first = 0;
        return str[first];
    }

    uchar32 lastChar(const string_t::view& str)
    {
        s32 const last = str.size() - 1;
        return str[last];
    }

    bool startsWith(const string_t::view& str, string_t::view const& start)
    {
        string_t::view v = str(0, start.size());
        if (!v.is_empty())
            return (v == start);
        return false;
    }

    bool endsWith(const string_t::view& str, string_t::view const& end)
    {
        string_t::view v = str(str.size() - end.size(), str.size());
        if (!v.is_empty())
            return (v == end);
        return false;
    }

    string_t::view find(string_t& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return xview::get_default();
    }

    string_t::view find(string_t::view& str, uchar32 find)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const c = str[i];
            if (c == find)
                return str(i, i + 1);
        }
        return xview::get_default();
    }

    string_t::view find(string_t& str, const string_t::view& find)
    {
        string_t::view v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v;
            }
            if (!xview::move_view(v, 1))
                break;
        }
        return xview::get_default();
    }

    string_t::view find(string_t::view& str, const string_t::view& find)
    {
        string_t::view v = str(0, find.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v;
            }
            if (!xview::move_view(v, 1))
                break;
        }
        return xview::get_default();
    }

    string_t::view findLast(const string_t::view& str, const string_t::view& find)
    {
        string_t::view v = str(str.size() - find.size(), str.size());
        while (!v.is_empty())
        {
            if (v == find)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return v;
            }
            if (!xview::move_view(v, -1))
                break;
        }
        return xview::get_default();
    }

    string_t::view findOneOf(const string_t::view& str, const string_t::view& charset)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = str[i];
            for (s32 j = 0; j < charset.size(); j++)
            {
                uchar32 const fc = charset[i];
                if (sc == fc)
                {
                    return str(i, i + 1);
                }
            }
        }
        return xview::get_default();
    }

    string_t::view findOneOfLast(const string_t::view& str, const string_t::view& charset)
    {
        for (s32 i = str.size() - 1; i >= 0; i--)
        {
            uchar32 const sc = str[i];
            for (s32 j = 0; j < charset.size(); j++)
            {
                uchar32 const fc = charset[i];
                if (sc == fc)
                {
                    return str(i, i + 1);
                }
            }
        }
        return xview::get_default();
    }

    s32 compare(const string_t::view& lhs, const string_t::view& rhs)
    {
        if (lhs.size() < rhs.size())
            return -1;
        if (lhs.size() > rhs.size())
            return 1;

        for (s32 i = 0; i < lhs.size(); i++)
        {
            uchar32 const lc = xview::get_char_unsafe(lhs, i);
            uchar32 const rc = xview::get_char_unsafe(rhs, i);
            if (lc < rc)
                return -1;
            else if (lc > rc)
                return 1;
        }
        return 0;
    }

    bool isEqual(const string_t::view& lhs, const string_t::view& rhs) { return compare(lhs, rhs) == 0; }

    bool contains(const string_t::view& str, const string_t::view& contains)
    {
        string_t::view v = str(0, contains.size());
        while (!v.is_empty())
        {
            if (v == contains)
            {
                // So here we have a view with the size of the @find string on
                // string @str that matches the string @find.
                return true;
            }
            if (!xview::move_view(v, 1))
                break;
        }
        return false;
    }

    bool contains(const string_t::view& str, uchar32 contains)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32 const sc = xview::get_char_unsafe(str, i);
            if (sc == contains)
            {
                return true;
            }
        }
        return false;
    }

    void concatenate_repeat(string_t& str, string_t::view const& con, s32 ntimes)
    {
        s32 const len = str.size() + (con.size() * ntimes) + 1;
        xview::resize(str, len);
        for (s32 i = 0; i < ntimes; ++i)
        {
            concatenate(str, con);
        }
    }

    s32 format(string_t& str, string_t::view const& format, const va_list_t& args)
    {
        str.clear();
        s32 len = vcprintf(xview::get_runes(format), args);
        xview::resize(str, len);
        vsprintf(xview::get_runes(str), xview::get_runes(format), args);
        return 0;
    }

    s32 formatAdd(string_t& str, string_t::view const& format, const va_list_t& args)
    {
        s32 len = vcprintf(xview::get_runes(format), args);
        xview::resize(str, len);
        vsprintf(xview::get_runes(str), xview::get_runes(format), args);
        return 0;
    }

    void insert(string_t& str, string_t::view const& pos, string_t::view const& insert) { xview::insert(str, pos, insert); }

    void insert_after(string_t& str, string_t::view const& pos, string_t::view const& insert)
    {
        string_t::view after = xview::select_after_excluded(str.full(), pos);
        xview::insert(str, after, insert);
    }

    void remove(string_t& str, string_t::view const& selection) { xview::remove(str, selection); }

    void find_remove(string_t& str, const string_t::view& _find)
    {
        string_t::view strvw = str.full();
        string_t::view sel   = find(strvw, _find);
        if (sel.is_empty() == false)
        {
            xview::remove(str, sel);
        }
    }

    void find_replace(string_t& str, const string_t::view& find, const string_t::view& replace) { xview::find_replace(str, find, replace); }

    void remove_any(string_t& str, const string_t::view& any) { xview::remove_any(str, any); }

    void replace_any(string_t::view& str, const string_t::view& any, uchar32 with)
    {
        // Replace any of the characters in @charset from @str with character @with
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 const c = xview::get_char_unsafe(str, i);
            if (contains(any, c))
            {
                xview::set_char_unsafe(str, i, with);
            }
        }
    }

    void upper(string_t::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32* cp = xview::get_ptr_unsafe(str, i);
            *cp         = to_upper(*cp);
        }
    }

    void lower(string_t::view& str)
    {
        for (s32 i = 0; i < str.size(); i++)
        {
            uchar32* cp = xview::get_ptr_unsafe(str, i);
            *cp         = to_lower(*cp);
        }
    }

    void capitalize(string_t::view& str)
    {
        // Standard separator is ' '
        bool prev_is_space = true;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = xview::get_char_unsafe(str, i);
            uchar32 d = c;
            if (is_alpha(c))
            {
                if (prev_is_space)
                {
                    c = to_upper(c);
                }
                else
                {
                    c = to_lower(c);
                }
            }
            else
            {
                prev_is_space = is_space(c);
            }

            if (c != d)
            {
                xview::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    void capitalize(string_t::view& str, string_t::view const& separators)
    {
        bool prev_is_space = false;
        s32  i             = 0;
        while (i < str.size())
        {
            uchar32 c = xview::get_char_unsafe(str, i);
            uchar32 d = c;
            if (is_alpha(c))
            {
                if (prev_is_space)
                {
                    c = to_upper(c);
                }
                else
                {
                    c = to_lower(c);
                }
            }
            else
            {
                prev_is_space = false;
                for (s32 j = 0; j < separators.size(); j++)
                {
                    if (separators[j] == c)
                    {
                        prev_is_space = true;
                        break;
                    }
                }
            }
            if (c != d)
            {
                xview::set_char_unsafe(str, i, c);
            }
            i++;
        }
    }

    // Trim does nothing more than narrowing the <from, to>, nothing is actually removed
    // from the actual underlying string data.
    void trim(string_t::view& str)
    {
        trimLeft(str);
        trimRight(str);
    }

    void trimLeft(string_t::view& str)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    xview::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t::view& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, last - i);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            {
                if (i > 0)
                {
                    xview::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(string_t::view& str, uchar32 r)
    {
        trimLeft(str, r);
        trimRight(str, r);
    }

    void trimLeft(string_t::view& str, uchar32 r)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, i);
            if (c != r)
            {
                if (i > 0)
                {
                    xview::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t::view& str, uchar32 r)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, last - i);
            if (c != r)
            {
                if (i > 0)
                {
                    xview::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trim(string_t::view& str, string_t::view const& set)
    {
        trimLeft(str, set);
        trimRight(str, set);
    }

    void trimLeft(string_t::view& str, string_t::view const& set)
    {
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    xview::narrow_view(str, -i);
                }
                return;
            }
        }
    }

    void trimRight(string_t::view& str, string_t::view const& set)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < str.size(); ++i)
        {
            uchar32 c = xview::get_char_unsafe(str, last - i);
            if (!contains(set, c))
            {
                if (i > 0)
                {
                    xview::narrow_view(str, i);
                }
                return;
            }
        }
    }

    void trimQuotes(string_t::view& str) { trimDelimiters(str, '"', '"'); }

    void trimQuotes(string_t::view& str, uchar32 quote) { trimDelimiters(str, quote, quote); }

    void trimDelimiters(string_t::view& str, uchar32 left, uchar32 right)
    {
        trimLeft(str, left);
        trimRight(str, right);
    }

    void reverse(string_t::view& str)
    {
        s32 const last = str.size() - 1;
        for (s32 i = 0; i < (last - i); ++i)
        {
            uchar32 l = xview::get_char_unsafe(str, i);
            uchar32 r = xview::get_char_unsafe(str, last - i);
            xview::set_char_unsafe(str, i, r);
            xview::set_char_unsafe(str, last - i, l);
        }
    }

    bool splitOn(string_t::view& str, uchar32 inChar, string_t::view& outLeft, string_t::view& outRight)
    {
        outLeft = selectUntil(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOn(string_t::view& str, string_t::view& inStr, string_t::view& outLeft, string_t::view& outRight)
    {
        outLeft = selectUntil(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    bool splitOnLast(string_t::view& str, uchar32 inChar, string_t::view& outLeft, string_t::view& outRight)
    {
        outLeft = selectUntilLast(str, inChar);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size(), str.size());
        trimRight(outLeft, inChar);
        return true;
    }

    bool splitOnLast(string_t::view& str, string_t::view& inStr, string_t::view& outLeft, string_t::view& outRight)
    {
        outLeft = selectUntilLast(str, inStr);
        if (outLeft.is_empty())
            return false;
        outRight = str(outLeft.size() + inStr.size(), str.size());
        return true;
    }

    void concatenate(string_t& str, const string_t::view& con) { xview::resize(str, str.size() + con.size() + 1); }

    //------------------------------------------------------------------------------
    static void user_case_for_string()
    {
        string_t str("This is an ascii string that will be converted to UTF-32");

        string_t::view strvw  = str.full();
        string_t::view substr = find(strvw, string_t("ascii"));
        upper(substr);

        find_remove(str, string_t("converted "));
    }

} // namespace xcore
