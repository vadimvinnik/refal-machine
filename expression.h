#ifndef _REFAL_MACHINE_EXPRESSION_H_
#define _REFAL_MACHINE_EXPRESSION_H_

// C std
#include <assert.h>

// C++ std
#include <algorithm>
#include <initializer_list>
#include <list>
#include <numeric>
#include <sstream>
#include <string>

// 3rd party
#include <boost/intrusive_ptr.hpp>

// this project
#include "directional_iterator.h"

class Expression;
class Term;
class TermEnumerator;
class ConcatenationNode;

typedef boost::intrusive_ptr<Term const> PCTerm;
typedef boost::intrusive_ptr<Expression const> PCExpression;
typedef std::list<PCExpression> ExpressionList;

// to support boost::intrusive_ptr
template<typename T>
inline void intrusive_ptr_add_ref(T* p) {
    ++p->m_refcount;
}

template<typename T>
inline void intrusive_ptr_release(T* p) {
    if(0 == --p->m_refcount)
        delete p;
}


class Expression {
protected:
  class TermEnumeratorBase {
  public:
    virtual ~TermEnumeratorBase() {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const = 0;
    bool operator==(const TermEnumeratorBase& x) const { return isEqualTo(x); }
    bool operator!=(const TermEnumeratorBase& x) const { return !isEqualTo(x); }

    virtual void toNext() = 0;
    TermEnumeratorBase& operator++() { toNext(); return *this; }

    virtual PCTerm current() const = 0;
    operator PCTerm() const { return current(); }
    Term const& operator*() const { return *current(); }
    PCTerm operator->() const { return current(); }

  private:
    template<typename T> friend void intrusive_ptr_add_ref(T* expr);
    template<typename T> friend void intrusive_ptr_release(T* expr);
    mutable long m_refcount;
  };

  typedef boost::intrusive_ptr<TermEnumeratorBase> PTermEnumeratorBase;

  // restrict enumerators that can be wrapped into TermEnumerator
  // to avoid multiple levels of indirection
  class TermEnumeratorWorkerBase : public TermEnumeratorBase {};
  typedef boost::intrusive_ptr<TermEnumeratorWorkerBase> PTermEnumeratorWorkerBase;
  
public:
  /* this wrapper of a real worker object is needed because
   * begin() and end() must return objects, not (smart) pointers,
   * but term enumerators are polymorphic, and their base class
   * is abstract */
  class TermEnumerator : public TermEnumeratorBase {
  public:
    TermEnumerator(PTermEnumeratorWorkerBase delegee) : m_delegee(delegee) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto xAsTermEnumerator = dynamic_cast<TermEnumerator const*>(&x);
      return nullptr != xAsTermEnumerator && m_delegee->isEqualTo(*xAsTermEnumerator->m_delegee);
    }

    virtual void toNext() { m_delegee->toNext(); }
    virtual PCTerm current() const { return m_delegee->current(); }

  private:
    PTermEnumeratorWorkerBase m_delegee;
  };

  virtual ~Expression() {}
  virtual std::string toString() const = 0;
  virtual bool isEmpty() const { return 0 == termsCount(); }
  virtual int termsCount() const = 0;
  virtual TermEnumerator begin(LookupDirection direction) const { return TermEnumerator(beginImpl(direction)); }
  virtual TermEnumerator end(LookupDirection direction) const { return TermEnumerator(endImpl(direction)); }

protected:
  virtual PTermEnumeratorWorkerBase beginImpl(LookupDirection direction) const = 0;
  virtual PTermEnumeratorWorkerBase endImpl(LookupDirection direction) const = 0;

private:
  template<typename T> friend void intrusive_ptr_add_ref(T* expr);
  template<typename T> friend void intrusive_ptr_release(T* expr);

  mutable long m_refcount;
};

// A base class for all entities allowed to be used in concatenation.
// Restricting concatenation avoids growing of expression trees.
class ConcatenationNode : public Expression {
};

// A term is either a single symbol or any expression in parentheses.
// Obviously, a term considered as an expression contains just one
// term, namely itself.
class Term : public ConcatenationNode {
public:
  virtual bool isEmpty() const { return false; }
  virtual int termsCount() const { return 1; }

protected:
  // Enumerates an imaginary collection containing exactly one item
  class TermSelfEnumerator : public Expression::TermEnumeratorWorkerBase {
  public:
    TermSelfEnumerator(PCTerm target, bool finished) : m_target(target), m_finished(finished) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto te = dynamic_cast<TermSelfEnumerator const*>(&x);
      return nullptr != te && te->m_target == m_target && te->m_finished == m_finished;
    }

    virtual void toNext() {
      assert(!m_finished);
      m_finished = true;
    }

    virtual PCTerm current() const {
      assert(!m_finished);
      return m_target;
    }

  private:
    PCTerm m_target;
    bool m_finished;
  };

  virtual PTermEnumeratorWorkerBase beginImpl(LookupDirection direction) const { return createEnumerator(false); }
  virtual PTermEnumeratorWorkerBase endImpl(LookupDirection direction) const { return createEnumerator(true); }

private:
  PTermEnumeratorWorkerBase createEnumerator(bool finished) const {
    return PTermEnumeratorWorkerBase(new TermSelfEnumerator(PCTerm(this), finished));
  }
};

class Symbol : public Term {
public:
  Symbol(char symbol) : m_symbol(symbol) {}
  virtual std::string toString() const { return std::string(1, m_symbol); }

private:
  char m_symbol;
};

class Parenthesized : public Term {
public:
  Parenthesized(PCExpression inner) : m_inner(inner) {}
  virtual std::string toString() const { return "(" + m_inner->toString() + ")"; }
  PCExpression inner() const { return m_inner; }

private:
  PCExpression m_inner;
};

// Although a plain expression is iconceptually a concatenation of symbols,
// and there is no special treatment of strings, this class is needed to
// optimize memory usage: it avoids storing each symbol as a separate
// object of Symbol class.
class Literal : public ConcatenationNode {
public:
  Literal(std::string symbols) : m_symbols(symbols) {}
  virtual std::string toString() const { return m_symbols; }
  virtual bool isEmpty() const { return m_symbols.empty(); }
  virtual int termsCount() const { return m_symbols.length(); }

protected:
  typedef directional_iterator<std::string::const_iterator> string_directional_iterator;

  class SymbolEnumerator : public ConcatenationNode::TermEnumeratorWorkerBase {
  public:
    SymbolEnumerator(string_directional_iterator const& position) :
      m_position(position)
    {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto se = dynamic_cast<SymbolEnumerator const*>(&x);
      return nullptr != se && se->m_position == m_position;
    }

    virtual void toNext() {
      ++m_position;
      m_symbol = nullptr;
    }

    virtual PCTerm current() const {
      if (m_symbol == nullptr)
      {
        m_symbol = PCTerm(new Symbol(*m_position));
      }

      return m_symbol;
    }

  private:
    // since a symbol is not a standalone object but just a character
    // in a string, and since it has to look and behave like any other
    // Term object, let each SymbolEnumerator create its own Symbol
    // object and refer to it.
    mutable PCTerm m_symbol;
    string_directional_iterator m_position;
  };

  virtual PTermEnumeratorWorkerBase beginImpl(LookupDirection direction) const {
    return createEnumerator(Begin, direction);
  }

  virtual PTermEnumeratorWorkerBase endImpl(LookupDirection direction) const {
    return createEnumerator(End, direction);
  }

private:
  PTermEnumeratorWorkerBase createEnumerator(ContainerBound bound, LookupDirection direction) const {
    return PTermEnumeratorWorkerBase(
        new SymbolEnumerator(
          relative_container_bound<std::string, std::string::const_iterator>
            (m_symbols, bound, direction)));
  }

  std::string m_symbols;
};

class Concatenation : public Expression {
public:
  Concatenation(std::initializer_list<PCExpression> components) : m_components(components.begin(), components.end()) {}

  virtual std::string toString() const {
    std::ostringstream s;
    std::for_each(
        m_components.cbegin(),
        m_components.cend(),
        [&s](PCExpression const& pe) { s << pe->toString(); });
    return s.str();
  }

  virtual int termsCount() const {
    return std::accumulate(
        m_components.cbegin(),
        m_components.cend(),
        0,
        [](int s, const PCExpression& e) { return s + e->termsCount(); });
  }

  virtual bool isEmpty() const {
    return std::all_of(m_components.cbegin(), m_components.cend(), isPtrToEmpty);
  }

protected:
  typedef directional_iterator<ExpressionList::const_iterator> expression_list_directional_iterator;

  class ConcatenationTermEnumerator : public TermEnumeratorWorkerBase {
  public:
    ConcatenationTermEnumerator(
      LookupDirection direction,
      expression_list_directional_iterator const& current_component,
      expression_list_directional_iterator const& components_end
    ) :
      m_direction(direction),
      m_current_component(current_component), 
      m_components_end(components_end),
      m_current_term(nullptr)
    {
      findNextTerm();
    }

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto te = dynamic_cast<ConcatenationTermEnumerator const*>(&x);
      return
        nullptr != te &&
	te->m_current_component == m_current_component &&
	te->m_components_end == m_components_end &&
	te->isAtEnd() == isAtEnd() &&
	(isAtEnd() || te->m_current_term == m_current_term);
    }

    virtual void toNext() {
      assert(!isAtEnd());
      ++m_current_term;
      if (m_current_term == (*m_current_component)->end(m_direction)) {
        ++m_current_component;
        findNextTerm();
      }
    }

    virtual PCTerm current() const {
      assert(!isAtEnd());
      return m_current_term;
    }

  private:
    bool isAtEnd() const { return m_current_component == m_components_end; }

    void findNextTerm() {
      m_current_component = find_if_not(m_current_component, m_components_end, isPtrToEmpty);

      if (!isAtEnd()) {
        m_current_term = (*m_current_component)->begin(m_direction);
      }
    }

    LookupDirection m_direction;
    expression_list_directional_iterator m_current_component;
    expression_list_directional_iterator m_components_end;
    TermEnumerator m_current_term;
  };

  virtual PTermEnumeratorWorkerBase beginImpl(LookupDirection direction) const {
    assert(LeftToRight == direction || RightToLeft == direction);
    ExpressionList::const_iterator const bounds[] = { m_components.cbegin(), m_components.cend() };
    return PTermEnumeratorWorkerBase(
        new ConcatenationTermEnumerator(
          direction,
          expression_list_directional_iterator(direction, bounds[direction]),
          expression_list_directional_iterator(direction, bounds[1 - direction])));
  }

  virtual PTermEnumeratorWorkerBase endImpl(LookupDirection direction) const {
    auto end = LeftToRight == direction ? m_components.cend() : m_components.cbegin();
    auto directional_end = expression_list_directional_iterator(direction, end);
    return PTermEnumeratorWorkerBase(
        new ConcatenationTermEnumerator(
          direction,
          directional_end,
          directional_end));
  }

private:
  static bool isPtrToEmpty(PCExpression const& pe) { return pe->isEmpty(); }

  ExpressionList m_components;
};

class SubExpression : public Expression {
public:

private:
  boost::intrusive_ptr<Expression> m_target;
};

#endif

