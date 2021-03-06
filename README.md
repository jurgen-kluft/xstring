# xstring

Cross platform string library (still under construction)

This string has a UTF-32 encoding, it allows to have views on part of the string.
Since we now can have multiple views on the string we can do away with the user
managing indices. However the string may re-allocate when an operation is causing
it to resize; this includes 'insert', 'replace' and 'remove'.
When this happens all of the active views are automatically 'corrected'. A view
might get invalidated if the operation is 'removing' the part of the string that
the view is watching.

Instead we can do things like this:

``` c++
xstring str("This is an ascii converted to UTF-32 when constructed");

xstring::view ascii_substring = find(str, "ascii");
upper(ascii_substring);    // 'ascii' to 'ASCII'

// This inserts " string" into the main string at the location indicated by @ascii_substring
// which gets the string to resize, the view @ascii_substring will still be correct.
ascii_substring = insert_after(str, ascii_substring, xstring(" string"));

// So now @str = "This is an ASCII string converted to UTF-32 when constructed"
// And @ascii_substring = "ASCII"
```

One thing to keep in mind is that a view can be invalidated when the actual string is
resized due to insert, replace or remove calls, so know what you are doing. It is just
something to be aware of, invalidated views can still be used and will not cause any
crashes.

You can clone a string (string data is copied):

``` c++
xstring a_copy(str);
xstring::view to_remove = find(a_copy, xstring(" when constructed"));
remove(a_copy, to_remove);  // This removes the substring from the main string
// So now @a_copy = "This is an ASCII string converted to UTF-32"
// @to_remove is invalidated since that part does not exist anymore.

// If you want to lower-case the first letter of 'This'
xstring::view to_lower = a_copy(1);
lower(to_lower);
```
