# preface

There should be files for code auto-formatting tools in the directory.

Please use common sense and consult a good mature style guide, like [google's](https://google.github.io/styleguide/cppguide.html), even though a lot of points differ here.

# code style

### indentation

Indentation is 2 spaces. Lines are softly limited to 80 chars, meaning it's okay to go a bit further if necessary. Everything inside namespace is not indented. Labels in classes are not indented also.

```cpp
namespace example {
class a {
public:
  a();
};
};
```

### braces

Braces after statements, functions, namespaces, etc. are placed on the same line:

```cpp
int a() {
  if (true) {
```

### one-liners

Braces for one-line functions must be omitted. Also all blocks of code must be indented and must not be on the same line as statements, functions, etc.

```cpp
// bad:
int a() { f(); } // no
int b() {
  if (true) { f(); } // no
  if (true) f(); // no
  if (true) {
    f();
  }  // no
}
// good:
int a() {
  f(); // yes
}
int b() {
  if (true)
    f(); // yes
}
```

### lines

If you need to break a line, do it by operator first. Indents after line breaks are 2 units of indent, thus 4 spaces.

```cpp
// before:
void very_long_function(int a, int b) {
  if (a == 0 && b == 1)
    int c = a * 2 + b;
}
// after:
void very_long_function(int a
    , int b) {
  if (a == 0
      && b == 1)
    int c = a * 2
        + b;
}
```

### classes

Private members are prefixed with a single underscore. remember to use initializer lists. variables passed to the constructor in this case are prefixed with "n_" or only "n" for private members. if there're more than 3 variables to init, break each declaration to the separate line:

Prefer getters than public variables.

```cpp
class a {
  int _x, _y, _z;
public:
  int i; // only as an example 
  a(int n_x, int n_y, int n_z, int n_i)
      : _x(n_x)
      , _y(n_y)
      , _z(n_z)
      , i(n_i) {
    }
};
```

### evertything else

Extensions for source files and headers are .cc and .hh respectively.

Prefer code to be rather more verbose and readable than compact: for example if writing `if (condition == false)` is easier for a human to interpret, then do it.

Always put spaces around arithmetic operators: `a = b + c * 3`.

Remember to use `static` when declaring functions or variables that are not used outside the file.

Use `#pragma once` as a header guard.

Do not use RTTI. Exceptions should also be avoided, but not as strictly.

Use C++ style casts.

Use prefix form (++i) everywhere, except where postfix is necessary.

Don't forget to use `const`, const references and `constexpr` where appropriate. Use `const` rather than `#define`.

