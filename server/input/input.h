#ifndef INPUT_H
#define INPUT_H

#include <memory>
#include <string_view>

#include "string_param.hpp"

class InputImpl;
namespace qls {

class Input final {
public:
  Input();
  ~Input();

  void init();

  bool input(string_param command);

private:
  std::unique_ptr<InputImpl> m_impl;
};

} // namespace qls

#endif // !INPUT_H
