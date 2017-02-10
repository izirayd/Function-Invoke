# Function-Invoke c++ std11
Function Invoke it special list function and it name function, which can be run(invoke). Special feature is no arguments. 

Function-Invoke its good release, because it use minimum code. Analog Function Pointer, but Function Pointer have problem with create type for function with style ( typedef std::function<void(int32_t)> func_int32_t; ) and it only for methods void Foo(int32_t bar);  More argument, more code it bad. Function-Invoke don`t use Function Pointer types, don`t create problem with creating types.

Class: std::function_invoke
Class: std::any

For add function in list function, use method AddFunction(string Name, Function). For run function, use method Invoke(string Name, ...).

# Example

![Example Img](https://pp.vk.me/c639119/v639119877/7d2b/tn2T025bDuo.jpg)
