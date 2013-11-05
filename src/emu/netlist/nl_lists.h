// license:GPL-2.0+
// copyright-holders:Couriersud
/*
 * nllists.h
 *
 */

#ifndef NLLISTS_H_
#define NLLISTS_H_

#include "nl_config.h"

// ----------------------------------------------------------------------------------------
// netlist_list_t: a simple list
// ----------------------------------------------------------------------------------------


template <class _ListClass>
struct netlist_list_t
{
public:
	ATTR_COLD netlist_list_t(int numElements)
	{
		m_num_elements = numElements;
		m_list = new _ListClass[m_num_elements];
		m_ptr = m_list;
		m_ptr--;
	}
	ATTR_COLD ~netlist_list_t()
	{
		delete[] m_list;
	}
	ATTR_HOT inline void add(const _ListClass elem)
	{
		assert(m_ptr-m_list <= m_num_elements - 1);

		*(++m_ptr) = elem;
	}
	ATTR_HOT inline _ListClass *first() { return &m_list[0]; }
	ATTR_HOT inline _ListClass *last()  { return m_ptr; }
	ATTR_HOT inline _ListClass *item(int i) { return &m_list[i]; }
	ATTR_HOT inline int count() const { return m_ptr - m_list + 1; }
	ATTR_HOT inline bool empty() { return (m_ptr < m_list); }
	ATTR_HOT inline void clear() { m_ptr = m_list - 1; }
private:
	_ListClass * m_ptr;
	_ListClass * m_list;
	int m_num_elements;
	//_ListClass m_list[_NumElements];
};

// ----------------------------------------------------------------------------------------
// timed queue
// ----------------------------------------------------------------------------------------

template <class _Element, class _Time, int _Size>
class netlist_timed_queue1
{
public:

	struct entry_t
	{
	public:
		inline entry_t()
		: m_time(), m_object(NULL) {}
		inline entry_t(const _Time atime, _Element &elem) : m_time(atime), m_object(&elem) {}
		ATTR_HOT inline const _Time &time() const { return m_time; }
		ATTR_HOT inline _Element & object() const { return *m_object; }
	private:
		_Time m_time;
		_Element *m_object;
	};

	netlist_timed_queue1()
	{
		//m_list = global_alloc_array(entry_t, SIZE);
		clear();
	}

	ATTR_HOT inline bool is_empty() const { return (m_end == &m_list[0]); }
	ATTR_HOT inline bool is_not_empty() const { return (m_end > &m_list[0]); }

	ATTR_HOT ATTR_ALIGN inline void push(const entry_t &e)
	{
		if (is_empty() || (e.time() <= (m_end - 1)->time()))
		{
			*m_end++ = e;
			//inc_stat(m_prof_end);
		}
		else
		{
			entry_t * RESTRICT i = m_end++;
			while ((i>&m_list[0]) && (e.time() > (i-1)->time()) )
			{
				i--;
				*(i+1) = *i;
				//inc_stat(m_prof_sortmove);
			}
			*i = e;
			//inc_stat(m_prof_sort);
		}
		assert(m_end - m_list < _Size);
	}

	ATTR_HOT inline const entry_t pop()
	{
		return *--m_end;
	}

	ATTR_HOT inline const entry_t peek() const
	{
		return *(m_end-1);
	}

	ATTR_COLD void clear()
	{
		m_end = &m_list[0];
	}
	// profiling

	INT32   m_prof_start;
	INT32   m_prof_end;
	INT32   m_prof_sortmove;
	INT32   m_prof_sort;
private:

	entry_t * RESTRICT m_end;
	//entry_t *m_list;
	entry_t m_list[_Size];

};


#endif /* NLLISTS_H_ */
