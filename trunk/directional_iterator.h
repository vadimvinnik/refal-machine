#ifndef _DIRECTIONAL_ITERATOR_H_
#define _DIRECTIONAL_ITERATOR_H_

/*
Unfortunately, std::X::[const_]reverse_iterator is not assignable to a
respective std::X::[const_]iterator.

To use normal and reverse iteration modes interchangeably, determining
the mode at run time, a wrapper is needed.
*/

enum ContainerBound {
  Begin,
  End
};

enum LookupDirection {
  LeftToRight,
  RightToLeft
};

static inline LookupDirection reverse_direction(LookupDirection direction) {
  return static_cast<LookupDirection>(1 - direction);
}

// Begin of a container looked up in a reverse order is end of the
// initial container etc.
static inline ContainerBound relative_bound(ContainerBound bound, LookupDirection direction) {
  return static_cast<ContainerBound>(bound ^ direction);
}

template <typename TIterator>
class directional_iterator :
  public std::iterator<std::input_iterator_tag, typename TIterator::value_type> {
public:
  directional_iterator(LookupDirection direction, TIterator iterator) :
    m_direction(direction),
    m_iterator(iterator)
  {}

  bool operator==(directional_iterator const& x) const { return isEqualTo(x); }
  bool operator!=(directional_iterator const& x) const { return !isEqualTo(x); }

  directional_iterator& operator++() { move(LeftToRight); return *this; }
  directional_iterator& operator--() { move(RightToLeft); return *this; }
  typename TIterator::reference operator*() { return *current(); }
  typename TIterator::reference const& operator*() const { return *current(); }

private:
  bool isEqualTo(directional_iterator const& x) const {
    return m_direction == x.m_direction && m_iterator == x.m_iterator;
  }

  TIterator current() const {
    if (LeftToRight == m_direction)
      return m_iterator;
    else {
      auto prev = m_iterator;
      --prev;
      return prev;
    }
  }

  void move(LookupDirection relative_direction) {
    assert(LeftToRight == relative_direction || RightToLeft == relative_direction);
    if (relative_direction == m_direction)
      ++m_iterator;
    else
      --m_iterator;
  }


  LookupDirection m_direction;
  TIterator m_iterator;
};

template <typename TContainer, typename TIterator>
directional_iterator<TIterator> relative_container_bound(
    TContainer const& container,
    ContainerBound bound,
    LookupDirection direction)
{
  return directional_iterator<TIterator>(
      direction,
      Begin == relative_bound(bound, direction)
        ? container.begin()
        : container.end());
}

#endif
