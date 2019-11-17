#include "example_pimpl.h"
#include <string>
#include <cassert>

class pimpl {
public:
    const std::string& get_name() const noexcept { return name_; }
    void set_name(const std::string& name) { name_ = name; }

private:
    std::string name_;
};

example_pimpl::example_pimpl()
:    pimpl_(new pimpl)
{
}

example_pimpl::example_pimpl(char const * const name)
:    pimpl_(new pimpl)
{
    assert(name);
    pimpl_->set_name(name);
}

char const * const example_pimpl::get_name() const noexcept
{
    assert(!pimpl_->get_name().empty());
    return pimpl_->get_name().c_str();
}

void example_pimpl::set_name(char const * const name)
{
    assert(name);
    pimpl_->set_name(name);
}

example_pimpl::example_pimpl(example_pimpl&& rhs) noexcept = default;
example_pimpl::example_pimpl(const example_pimpl& rhs) = default;
example_pimpl& example_pimpl::operator=(example_pimpl&& rhs) noexcept = default;
example_pimpl& example_pimpl::operator=(const example_pimpl& rhs) = default;
example_pimpl::~example_pimpl() = default;