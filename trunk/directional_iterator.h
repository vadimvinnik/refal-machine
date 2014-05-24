#ifndef _DIRECTIONAL_ITERATOR_H_
#define _DIRECTIONAL_ITERATOR_H_

/*
Unfortunately, std::X::[const_]reverse_iterator is not assignable to a
respective std::X::[const_]iterator.

To use normal and reverse iteration modes interchangeably, determining
the mode at run time, a wrapper is needed.
*/

enum Direction {
  LeftToRight,
  RightToLeft
};

static inline Direction reverse_direction(Direction direction) {
  return static_cast<Direction>(1 - direction);
}

template <typename TIterator>
class directional_iterator {
public:
  directional_iterator(Direction direction, TIterator iterator) :
    m_direction(direction),
    m_iterator(iterator)
  {}

  bool operator==(directional_iterator const& x) const { return isEqualTo(x); }
  bool operator!=(directional_iterator const& x) const { return !isEqualTo(x); }

  directional_iterator& operator++() { move(LeftToRight); return *this; }
  directional_iterator& operator--() { move(RightToLeft); return *this; }
  typename TIterator::value_type& operator*() { return *current(); }
  typename TIterator::value_type const& operator*() const { return *current(); }

private:
  bool isEqualTo(directional_iterator const& x) const {
    return m_direction == x.m_direction && m_iterator == x.m_iterator;
  }

  TIterator const current() const {
    if (LeftToRight == m_direction)
      return m_iterator;
    else {
      auto prev = m_iterator;
      --prev;
      return prev;
    }
  }

  void move(Direction relative_direction) {
    assert(LeftToRight == relative_direction || RightToLeft == relative_direction);
    if (relative_direction == m_direction)
      ++m_iterator;
    else
      --m_iterator;
  }


  Direction m_direction;
  TIterator m_iterator;
};

template <typename TContainer>
directional_iterator<typename TContainer::iterator> bound(
    TContainer const& container,
    Direction relative_direction,
    Direction absolute_direction)
{
  return directional_iterator<typename TContainer::iterator>(
      relative_direction,
      absolute_direction == relative_direction
        ? container.begin()
        : container.end());
}

template <typename TContainer>
directional_iterator<typename TContainer::const_iterator> const_bound(
    TContainer const& container,
    Direction relative_direction,
    Direction absolute_direction)
{
  return directional_iterator<typename TContainer::const_iterator>(
      relative_direction,
      absolute_direction == relative_direction
        ? container.cbegin()
        : container.cend());
}

#endif

