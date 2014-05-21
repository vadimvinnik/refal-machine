#include <assert.h>

#include <algorithm>
#include <initializer_list>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>

#include "refcount_ptr.h"

class Expression;
class Term;
class TermEnumerator;
class ExpressionNode;

typedef refcount_ptr<Term const> PCTerm;
typedef refcount_ptr<Expression const> PCExpression;
typedef std::list<PCExpression> ExpressionList;

class Expression {
public:
  enum Direction {
    LeftToRight,
    RightToLeft
  };

protected:
  class TermEnumeratorBase;
  typedef refcount_ptr<TermEnumeratorBase> PTermEnumeratorBase;

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
    PCTerm operator->() const { return current(); }
  };
  
public:
  class TermEnumerator : public TermEnumeratorBase {
  public:
    TermEnumerator(PTermEnumeratorBase delegee) : m_delegee(delegee) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto xAsTermEnumerator = dynamic_cast<TermEnumerator const*>(&x);
      return nullptr != xAsTermEnumerator && m_delegee->isEqualTo(*xAsTermEnumerator->m_delegee);
    }

    virtual void toNext() { m_delegee->toNext(); }
    virtual PCTerm current() const { return m_delegee->current(); }

  private:
    PTermEnumeratorBase m_delegee;
  };

  virtual ~Expression() {}
  virtual std::string toString() const = 0;
  virtual bool isEmpty() const = 0;
  virtual TermEnumerator front() const { return TermEnumerator(frontImpl()); }
  virtual TermEnumerator back() const { return TermEnumerator(backImpl()); }

protected:
  virtual PTermEnumeratorBase frontImpl() const = 0;
  virtual PTermEnumeratorBase backImpl() const = 0;
};

class ExpressionNode : public Expression {
public:
  virtual int termsCount() const = 0;
};

/*
class Empty : public ExpressionNode {
public:
  class EmptyEnumerator : public Expression::TermEnumeratorBase {
  public:
    virtual bool isEqualTo(const TermEnumeratorBase& x) const { return nullptr != dynamic_cast<EmptyEnumerator const*>(&x); }
    virtual void toNext() { assert(false); }
    virtual PCTerm current() const { assert(false); return nullptr; }
  };

  virtual std::string toString() const { return std::string(); }
  virtual bool isEmpty() const { return true; }
  virtual int termsCount() const { return 0; }

protected:
  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new EmptyEnumerator()); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new EmptyEnumerator()); }
};
*/

class Term : public ExpressionNode {
public:
  virtual bool isEmpty() const { return false; }
  virtual int termsCount() const { return 1; }

protected:
  class TermSelfEnumerator : public Expression::TermEnumeratorBase {
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

  virtual PTermEnumeratorBase frontImpl() const { return createEnumerator(false); }
  virtual PTermEnumeratorBase backImpl() const { return createEnumerator(true); }

private:
  PTermEnumeratorBase createEnumerator(bool finished) const {
    return PTermEnumeratorBase(new TermSelfEnumerator(PCTerm(this), finished));
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
};

class Literal : public ExpressionNode {
public:
  Literal(std::string symbols) : m_symbols(symbols) {}
  virtual std::string toString() const { return m_symbols; }
  virtual bool isEmpty() const { return m_symbols.empty(); }
  virtual int termsCount() const { return m_symbols.length(); }

protected:
  class SymbolEnumerator : public ExpressionNode::TermEnumeratorBase {
  public:
    SymbolEnumerator(std::string::const_iterator position) : m_position(position) {}

    virtual bool isEqualTo(const TermEnumeratorBase& x) const {
      auto se = dynamic_cast<SymbolEnumerator const*>(&x);
      return nullptr != se && se->m_position == m_position;
    }

    virtual void toNext() {
      ++m_position;
    }

    virtual PCTerm current() const { return PCTerm(new Symbol(*m_position)); }

  private:
    std::string::const_iterator m_position;
  };

  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new SymbolEnumerator(m_symbols.cbegin())); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new SymbolEnumerator(m_symbols.cend())); }

private:
  std::string m_symbols;
};

class Concatenation : public Expression {
public:
  Concatenation(std::initializer_list<PCExpression> components) : m_components(components.begin(), components.end()) {}

  virtual std::string toString() const {
    std::ostringstream s;
    std::for_each(m_components.cbegin(), m_components.cend(), [&s](PCExpression const& pe) { s << pe->toString(); });
    return s.str();
  }

  virtual bool isEmpty() const {
    return std::all_of(m_components.cbegin(), m_components.cend(), isPtrToEmpty);
  }

protected:
  class ConcatenationTermEnumerator : public TermEnumeratorBase {
  public:
    ConcatenationTermEnumerator(
      ExpressionList::const_iterator const& current_component,
      ExpressionList::const_iterator const& components_end
    ) :
      m_current_component(current_component), 
      m_components_end(components_end),
      m_current_cerm(nullptr)
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
	(isAtEnd() || te->m_current_cerm == m_current_cerm);
    }

    virtual void toNext() {
      assert(!isAtEnd());
      ++m_current_cerm;
      if (m_current_cerm == (*m_current_component)->back()) {
        ++m_current_component;
        findNextTerm();
      }
    }

    virtual PCTerm current() const {
      assert(!isAtEnd());
      return m_current_cerm;
    }

  private:
    bool isAtEnd() const { return m_current_component == m_components_end; }

    void findNextTerm() {
      m_current_component = find_if_not(m_current_component, m_components_end, isPtrToEmpty);

      if (!isAtEnd()) {
        m_current_cerm = (*m_current_component)->front();
      }
    }

    ExpressionList::const_iterator m_current_component;
    ExpressionList::const_iterator m_components_end;
    TermEnumerator m_current_cerm;
  };

  virtual PTermEnumeratorBase frontImpl() const { return PTermEnumeratorBase(new ConcatenationTermEnumerator(m_components.cbegin(), m_components.cend())); }
  virtual PTermEnumeratorBase backImpl() const { return PTermEnumeratorBase(new ConcatenationTermEnumerator(m_components.cend(), m_components.cend())); }

private:
  static bool isPtrToEmpty(PCExpression const& pe) { return pe->isEmpty(); }
  ExpressionList m_components;
};

class SubExpression : public Expression {
public:

private:
  refcount_ptr<Expression> m_target;
};

