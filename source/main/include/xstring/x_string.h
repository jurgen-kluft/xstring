#ifndef __XSTRING_STRING_H__
#define __XSTRING_STRING_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xbase/x_runes.h"

//==============================================================================
// xstring, a UTF32 string class
//==============================================================================
namespace xcore
{
	class x_va;
	class x_va_list;
	class xalloc;

	class xstring
	{
	public:
		xstring();
		xstring(const char* str);
		xstring(xstring const& other);
		~xstring();

		struct view
		{
			~view();

			s32		size() const;
			bool	is_empty() const;
			xstring to_string() const;

			view  operator()(s32 to);
			view  operator()(s32 from, s32 to);
			view  operator()(s32 to) const;
			view  operator()(s32 from, s32 to) const;

			uchar32 operator[](s32 index) const;

			view& operator=(view const& other);

			bool operator == (view const& other) const;
			bool operator != (view const& other) const;

		protected:
			friend class xstring;
			friend class xview;
			view();

			void		  add(view** list);
			void		  rem();
			void		  invalidate();
			utf32::crunes get_runes() const;

			utf32::alloc*  m_alloc;
			utf32::runes   m_runes;
			bool		   m_const;
			s32			   m_from;
			s32			   m_size;
			mutable view** m_list;
			view*		   m_next;
			view*		   m_prev;
		};

		xstring(xstring::view const& left, xstring::view const& right);

		bool is_empty() const;
		s32  size() const;

		view full();
		view full() const;

		view operator()(s32 to);
		view operator()(s32 from, s32 to);

		view operator()(s32 to) const;
		view operator()(s32 from, s32 to) const;

		uchar32 operator[](s32 index);
		uchar32 operator[](s32 index) const;

		xstring& operator=(const xstring& other);
		xstring& operator=(const xstring::view& other);

		static utf32::alloc* s_allocator;

	protected:
		friend class xview;

		xstring(utf32::alloc* mem, s32 size);

		void release();
		void clone(utf32::runes const& str, utf32::alloc* allocator);

		utf32::alloc* m_allocator;
		utf32::runes  m_runes;
		mutable view* m_views;
	};

	bool isUpper(const xstring::view&);
	bool isLower(const xstring::view&);
	bool isCapitalized(const xstring::view&);
	bool isQuoted(const xstring::view&);
	bool isQuoted(const xstring::view&, uchar32 inQuote);
	bool isDelimited(const xstring::view&, uchar32 inLeft, uchar32 inRight);

	uchar32 firstChar(const xstring::view&);
	uchar32 lastChar(const xstring::view&);

	bool startsWith(const xstring::view&, xstring::view const& inStartStr);
	bool endsWith(const xstring::view&, xstring::view const& inEndStr);

	///@name Comparison
	s32  compare(const xstring::view& inLHS, const xstring::view& inRHS);
	bool isEqual(const xstring::view& inLHS, const xstring::view& inRHS);
	bool contains(const xstring::view& inStr, uchar32 inContains);
	bool contains(const xstring::view& inStr, const xstring::view& inContains);

	s32 format(xstring&, xstring::view const& formatString, const x_va_list& args);
	s32 formatAdd(xstring&, xstring::view const& formatString, const x_va_list& args);

	void upper(xstring::view& inStr);
	void lower(xstring::view& inStr);
	void capitalize(xstring::view& inStr);
	void capitalize(xstring::view& inStr, xstring::view const& inSeperators);

	xstring::view selectUntil(const xstring::view& inStr, uchar32 inFind);
	xstring::view selectUntil(const xstring::view& inStr, const xstring::view& inFind);

	xstring::view selectUntilLast(const xstring::view& inStr, uchar32 inFind);
	xstring::view selectUntilLast(const xstring::view& inStr, const xstring::view& inFind);

	xstring::view selectUntilIncluded(const xstring::view& inStr, const xstring::view& inFind);

	///@name Search/replace
	xstring::view find(xstring::view& inStr, uchar32 inFind);
	xstring::view find(xstring::view& inStr, const xstring::view& inFind);
	xstring::view findLast(xstring::view& inStr, const xstring::view& inFind);
	xstring::view findOneOf(xstring::view& inStr, const xstring::view& inFind);
	xstring::view findOneOfLast(xstring::view& inStr, const xstring::view& inFind);

	void insert(xstring&, xstring::view const& pos, xstring::view const& inString);

	void remove(xstring&, xstring::view const& selection);
	void find_remove(xstring& str, const xstring::view& find);
	void find_replace(xstring& inStr, xstring::view const& find, xstring::view const& replace);
	void remove_any(xstring& str, const xstring::view& inAny);
	void replace_any(xstring&, xstring::view const& inAny, uchar32 inWith);

	void trim(xstring::view&);
	void trimLeft(xstring::view&);
	void trimRight(xstring::view&);
	void trim(xstring::view&, uchar32 inChar);
	void trimLeft(xstring::view&, uchar32 inChar);
	void trimRight(xstring::view&, uchar32 inChar);
	void trim(xstring::view&, xstring::view const& inCharSet);
	void trimLeft(xstring::view&, xstring::view const& inCharSet);
	void trimRight(xstring::view&, xstring::view const& inCharSet);
	void trimQuotes(xstring::view&);
	void trimQuotes(xstring::view&, uchar32 quote);
	void trimDelimiters(xstring::view&, uchar32 inLeft, uchar32 inRight);

	void reverse(xstring::view&);

	bool splitOn(xstring::view&, uchar32 inChar, xstring::view& outLeft, xstring::view& outRight);
	bool splitOn(xstring::view&, xstring::view& inStr, xstring::view& outLeft, xstring::view& outRight);
	bool splitOnLast(xstring::view&, uchar32 inChar, xstring::view& outLeft, xstring::view& outRight);
	bool splitOnLast(xstring::view&, xstring::view& inStr, xstring::view& outLeft, xstring::view& outRight);

	void concatenate(xstring& str, const xstring::view& con);
	void concatenate_repeat(xstring&, xstring::view const& con, s32 ntimes);

	// Global xstring operators

	inline xstring operator+(const xstring& left, const xstring& right) { return xstring(left.full(), right.full()); }
}

#endif ///< __XSTRING_STRING_H__