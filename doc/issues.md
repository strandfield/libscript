

# Issues

This is a list of known issues without reasonable solutions yet.

## Inheriting native class

Objects instantiated from classes defined directly in C++ cannot be 
converted to an `Object` using the `toObject()` method of `Value`.

This is a problem because the user might want to inherit such class.
As for now, the system assumes that the `Object` is created by the mother-class'
constructor. This would result in a runtime error (or some segfault)...

Even worse, a class might have a user-defined constructor, which would 
create an `Object`, and a constructor added in C++ after the class was 
created. Depending on which constructor is called, the `Object` will not 
always be present.

This is not a problem if the class declares no non-static data-members.
But that case should be considered rare (or at least not the most common one).

One possibility would be to force the implementer to declare a `getMember()` 
and a `setMember()` method in the class `ValueStruct` 
(this class is scarcely visible in the C++ API).

```cpp
struct ValueStruct
{
  Value getMember(int index) const;
  void setMember(int index, const Value & val);
}
```

This is perhaps the simplest possibility, and it moves the responsability away 
from the library: it is the implementer who must handle all cases (or to be more 
precise all cases that he will encounter). 
By default, we could declare `final` every classes provided by `libscript`, and thus 
effectively solve the problem on the library part.

Another relevant question is: is it legit do inherit from a native class ? 
Does it make sense to inherit `String` or `Array` ?