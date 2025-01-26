#pragma once

// C/C+
#include <string>

#define ADD_OPTION(T, st_name, op_name, doc)                                 \
  def(#op_name, (T const &(st_name::*)() const) & st_name::op_name,          \
      py::return_value_policy::reference, doc)                               \
      .def(#op_name, (st_name & (st_name::*)(const T &)) & st_name::op_name, \
           py::return_value_policy::reference, doc)

#define ADD_DISORT_MODULE(m_name, op_name)                     \
  torch::python::bind_module<disort::m_name##Impl>(m, #m_name) \
      .def(py::init<>(), R"(Construct a new default module.)") \
      .def(py::init<disort::op_name>(), R"(                     \
        Construct a new module with options)")                 \
      .def_readonly("options", &disort::m_name##Impl::options) \
      .def("__repr__",                                         \
           [](const disort::m_name##Impl &a) {                 \
             return fmt::format(#m_name "{}", a.options);      \
           })                                                  \
      .def("module",                                           \
           [](disort::m_name##Impl &self, std::string name) {  \
             return self.named_modules()[name];                \
           })                                                  \
      .def("buffer",                                           \
           [](disort::m_name##Impl &self, std::string name) {  \
             return self.named_buffers()[name];                \
           })                                                  \
      .def("forward", &disort::m_name##Impl::forward)
