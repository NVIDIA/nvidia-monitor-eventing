

# Rationale

Device id mapping language was devised to compactly represent a mapping between integer indexes and textual device names. This functionality is useful, from main daemon through wrappers to configuration files generating scripts. This is no surprise considering that between SMBPBI hardware interface and the Redfish interface, both using different names for the same devices with different indexing scheme, for example:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">SMBPBI</th>
<th scope="col" class="org-left">Redfish</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">`GPU0`</td>
<td class="org-left">`GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">`GPU1`</td>
<td class="org-left">`GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">`GPU2`</td>
<td class="org-left">`GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">`GPU3`</td>
<td class="org-left">`GPU_SXM_8`</td>
</tr>


<tr>
<td class="org-left">`GPU4`</td>
<td class="org-left">`GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">`GPU5`</td>
<td class="org-left">`GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">`GPU6`</td>
<td class="org-left">`GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">`GPU7`</td>
<td class="org-left">`GPU_SXM_4`</td>
</tr>
</tbody>
</table>

On top of that monitor-eventing is the controller of association relations between different devices and expressing them pose a challange, even if both sides are named using the same convention, for example the association between NVSwitches and Hot Swap Controllers:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">NVSwitch Redfish id</th>
<th scope="col" class="org-left">Hot Swap Controller Redfish id</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">`NVSwitch_0`</td>
<td class="org-left">`HSC_8`</td>
</tr>


<tr>
<td class="org-left">`NVSwitch_1`</td>
<td class="org-left">`HSC_8`</td>
</tr>


<tr>
<td class="org-left">`NVSwitch_2`</td>
<td class="org-left">`HSC_9`</td>
</tr>


<tr>
<td class="org-left">`NVSwitch_3`</td>
<td class="org-left">`HSC_9`</td>
</tr>
</tbody>
</table>

Device id mapping language provides the fundamental building block to easily express both of these kind of relations.


# Implementations

The language has two implementations:

1.  Python implementation `tools/device-id-lang/ids.py` to be used as a standalone CLI program as well as a python module to use in other scripts.
2.  C++ implementation in the form of set of classes to be used directly by the daemon's code, defined in the `device_id` namespace, in the files
    -   `src/device_id.cpp`,
    -   `include/device_id.hpp`.

The python version serves as a prototyping lab and may provide a richer set of features, but for all intents and purposes the languages defined by these two implementations are equivalent.


# Python parser `ids.py`

The fastest way to get the grip of the device id mapping language is to use the `ids.py` python program. It provides CLI for parsing expressions, printing the full mappings, evaluating the pattern for the given indexes, and so on.


## CLI program

This section introduces the basics of the language along with the `ids.py` program.


<a id="org1fed7a9"></a>

### Expanding device id pattern

To print all the values a pattern represents simply pass it as the first argument without any other options:

    $ python3 ids.py "GPU_SXM_[1-8]"
    GPU_SXM_1
    GPU_SXM_2
    GPU_SXM_3
    GPU_SXM_4
    GPU_SXM_5
    GPU_SXM_6
    GPU_SXM_7
    GPU_SXM_8

This shows how the bracket contents `1-8` are interpreted - as a range of values from `1` to `8`, which are interpolated into the containing string. The string may contain arbitrary characters, except for `[` and `]` (will be covered in the next version).

    $ python3 ids.py 'Change the gear
    to [1-6]...'
    Change the gear
    to 1...
    Change the gear
    to 2...
    Change the gear
    to 3...
    Change the gear
    to 4...
    Change the gear
    to 5...
    Change the gear
    to 6...

The numbers specifying the range cannot be negative. If the right number is lower than the left one the span is interpreted as empty which always results in an empty set of expanded values

    $ python3 ids.py "[5-0] nothing here!"

The pattern may contain multiple brackets in which case each of them is expanded independently (as long as there are no input position specified - see [Input position indexes](#org6c7b8c0) for more details).

    $ python3 ids.py "[0-2]--[17-19]"
    0--17
    0--18
    0--19
    1--17
    1--18
    1--19
    2--17
    2--18
    2--19


### Listing the mapping

The actual interpretation of a device id pattern is not really a set of strings it can expand to, but a *mapping* from numeric indexes into such expanded strings. (Thinking about it in this way provides many benefits, the most important being [evaluation of a device id pattern](#org83ab02a), to be discussed in a moment. Meanwhile the set of expanded strings are still easily accessible conceptually - it's simply the mapping's values set, or in other words its *image*)

To print the full mapping use the `--map` option:

    $ python3 ids.py --map "GPU_SXM_[1-8]"
    1:	GPU_SXM_1
    2:	GPU_SXM_2
    3:	GPU_SXM_3
    4:	GPU_SXM_4
    5:	GPU_SXM_5
    6:	GPU_SXM_6
    7:	GPU_SXM_7
    8:	GPU_SXM_8
    $ python3 ids.py --map "[0-2]--[17-19]"
    0 17:	0--17
    0 18:	0--18
    0 19:	0--19
    1 17:	1--17
    1 18:	1--18
    1 19:	1--19
    2 17:	2--17
    2 18:	2--18
    2 19:	2--19

This shows that the arguments of patterns are not really numbers but tuples of numbers. In case the pattern contains only one bracket the tuples are simply of size 1. When the pattern doesn't contain any number there is always exactly one argument - the empty tuple.

    $ python3 ids.py --map "I'll say it just once"
    :	I'll say it just once


<a id="org83ab02a"></a>

### Evaluating a device id pattern

Since pattern is a mapping it can be evaluated "at point", the point in this case being a tuple of numbers. To do it simply provide them after the pattern.

    $ python3 ids.py "GPU_SXM_[1-8]/NVLink_[0-11]" 2 5
    GPU_SXM_2/NVLink_5

Any additional numbers not needed in the pattern are ignored

    $ python3 ids.py "GPU_SXM_[1-8]/NVLink_[0-11]" 2 5 7 11
    GPU_SXM_2/NVLink_5

Not all the required numbers must be provided. In case there is not enough of them to evaluate the pattern into a single string it will be evaluated partially into a set of all possible expansions which have the corresponding brackets fixed on the provided value:

    $ python3 ids.py "GPU_SXM_[1-8]/NVLink_[0-11]" 2
    GPU_SXM_2/NVLink_0
    GPU_SXM_2/NVLink_1
    GPU_SXM_2/NVLink_2
    GPU_SXM_2/NVLink_3
    GPU_SXM_2/NVLink_4
    GPU_SXM_2/NVLink_5
    GPU_SXM_2/NVLink_6
    GPU_SXM_2/NVLink_7
    GPU_SXM_2/NVLink_8
    GPU_SXM_2/NVLink_9
    GPU_SXM_2/NVLink_10
    GPU_SXM_2/NVLink_11

The `--map` option works here as well, showing nicely what's happening ("currying").

    $ python3 ids.py --map "GPU_SXM_[1-8]/NVLink_[0-11]" 2
    2 0:	GPU_SXM_2/NVLink_0
    2 1:	GPU_SXM_2/NVLink_1
    2 2:	GPU_SXM_2/NVLink_2
    2 3:	GPU_SXM_2/NVLink_3
    2 4:	GPU_SXM_2/NVLink_4
    2 5:	GPU_SXM_2/NVLink_5
    2 6:	GPU_SXM_2/NVLink_6
    2 7:	GPU_SXM_2/NVLink_7
    2 8:	GPU_SXM_2/NVLink_8
    2 9:	GPU_SXM_2/NVLink_9
    2 10:	GPU_SXM_2/NVLink_10
    2 11:	GPU_SXM_2/NVLink_11

This is consistent with the [full expansion of a device id pattern](#org1fed7a9), which is simply a partial evaluation without any arguments.

What if we wanted to fix a second axis on, let's say `10`, and iterate the first one? A special value of `_` can be used.

    $ python3 ids.py "GPU_SXM_[1-8]/NVLink_[0-11]" _ 10
    1 10:	GPU_SXM_1/NVLink_10
    2 10:	GPU_SXM_2/NVLink_10
    3 10:	GPU_SXM_3/NVLink_10
    4 10:	GPU_SXM_4/NVLink_10
    5 10:	GPU_SXM_5/NVLink_10
    6 10:	GPU_SXM_6/NVLink_10
    7 10:	GPU_SXM_7/NVLink_10
    8 10:	GPU_SXM_8/NVLink_10


### Matching a string against a pattern

With the `--invert` option the mapping specified by the pattern is inverted, which means the strings are mapped to the corresponding index tuples. This can be then evaluated at point - this time a string - in the similar manner to how it was done previously.

    $ python3 ids.py --invert "GPU_SXM_[1-8]/NVLink_[0-17]" "GPU_SXM_3/NVLink_9"
    3 9

The tuple printed can be used as an input for another pattern. In this way we can easily obtain an instance of some pattern based on an instance of another one. For example, let's say we have the dbus object path on which an event occured, stored in a variable `objPath`

    $ echo "${objPath}"
    /xyz/openbmc_project/inventory/system/processors/GPU_SXM_2/Ports/NVLink_11

We expect this path to match the pattern

    /xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]/Ports/NVLink_[0-17]

We can obtain the redfish message args matching the pattern

    GPU_SXM_[1-8] NVLink_[0-17]

by calling

    $ python3 ids.py "GPU_SXM_[1-8] NVLink_[0-17]" \
      $(python3 ids.py --invert \
                "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]/Ports/NVLink_[0-17]" \
                ${objPath})
    GPU_SXM_2 NVLink_11


## Python module

File `ids.py` can be used as a module. All the funcionality provided in the CLI will be available in the form of object methods.


### Importing the module

To use the module in our script, let's say in `tools/some-tool/some-script.py`, it must be imported with

    import ids

We must ensure python is able to find it. There are many ways to do it and all of them are described at <https://docs.python.org/3/reference/import.html>. The simplest way is to use `PYTHONPATH` variable. Assuming the current directory is `tools/some-tool` the following call should ensure the `ids` package is imported properly

    PYTHONPATH=../device-id-lang python3 some-script.py


### Pattern parsing and iterating the mapping

Use `ids.parse()` function to obtain the `DeviceIdPattern` object. Then `pairs()` function can be called to iterate over the whole mapping:

    import ids

    pat = ids.parse("=<{[0-1]-[0-1]-[0-1]}>=")

    for arg, value in pat.pairs():
        print(f"{arg} -> '{value}'")

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    (0, 0, 0) -> '=<{0-0-0}>='
    (0, 0, 1) -> '=<{0-0-1}>='
    (0, 1, 0) -> '=<{0-1-0}>='
    (0, 1, 1) -> '=<{0-1-1}>='
    (1, 0, 0) -> '=<{1-0-0}>='
    (1, 0, 1) -> '=<{1-0-1}>='
    (1, 1, 0) -> '=<{1-1-0}>='
    (1, 1, 1) -> '=<{1-1-1}>='

The `pairs()` function returns an *iterator* of argument-value pairs, not a dictionary. To convert it to a dictionary just pass the iterator to `dict` constructor.

    import ids
    import pprint

    pat = ids.parse("=<{[0-1]-[0-1]-[0-1]}>=")

    patDict = dict(pat.pairs())
    pprint.pp(patDict)

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    {(0, 0, 0): '=<{0-0-0}>=',
     (0, 0, 1): '=<{0-0-1}>=',
     (0, 1, 0): '=<{0-1-0}>=',
     (0, 1, 1): '=<{0-1-1}>=',
     (1, 0, 0): '=<{1-0-0}>=',
     (1, 0, 1): '=<{1-0-1}>=',
     (1, 1, 0): '=<{1-1-0}>=',
     (1, 1, 1): '=<{1-1-1}>='}

The obtained dictionary can then be used to directly access the values by index tuples.

    key = (0, 1, 0)
    value = patDict[key]
    print(f"Value of {key}: '{value}'")

Output:

    Value of (0, 1, 0): '=<{0-1-0}>='


### Iterating the values (expanding the pattern)

Similarly to iterating the argument-value pairs we can iterate over the values of the mapping only with the `values()` function:

    import ids

    pat = ids.parse("=<{[0-1]-[0-1]-[0-1]}>=")

    for value in pat.values():
        print(f"'{value}'")

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    '=<{0-0-0}>='
    '=<{0-0-1}>='
    '=<{0-1-0}>='
    '=<{0-1-1}>='
    '=<{1-0-0}>='
    '=<{1-0-1}>='
    '=<{1-1-0}>='
    '=<{1-1-1}>='

Just as with `pairs()` the `values()` function returns an iterator. To obtain an indexable list it must be passed to the `list` constructor.

    import ids
    import pprint

    pat = ids.parse("=<{[0-1]-[0-1]-[0-1]}>=")

    valuesList = list(pat.values())
    pprint.pp(valuesList)

    print(f"Fourth element: {valuesList[3]}")

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    ['=<{0-0-0}>=',
     '=<{0-0-1}>=',
     '=<{0-1-0}>=',
     '=<{0-1-1}>=',
     '=<{1-0-0}>=',
     '=<{1-0-1}>=',
     '=<{1-1-0}>=',
     '=<{1-1-1}>=']
    Fourth element: =<{0-1-1}>=


### Evaluating the pattern

The `values()` function works in the same way as [evaluating a device id pattern](#org83ab02a) on a command line - calling it without any argument is equivalent to calling it with an empty tuple `()` argument and the resulting iterator then iterates over all possible pattern instantiations. Providing some numbers in the tuple will narrow down the resulting values set, for example

    import ids
    import pprint

    pat = ids.parse("=<{[0-1]-[0-1]-[0-1]}>=")

    for value in pat.values((0,)):
        print(f"'{value}'")

will print

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    '=<{0-0-0}>='
    '=<{0-0-1}>='
    '=<{0-1-0}>='
    '=<{0-1-1}>='

This program is equivalent to the call

    $ python3 ids.py '=<{[0-1]-[0-1]-[0-1]}>=' 0
    =<{0-0-0}>=
    =<{0-0-1}>=
    =<{0-1-0}>=
    =<{0-1-1}>=

The analogue of the `_` syntax is the `None` object, so to achieve in python what on CLI would look like

    $ python3 ids.py '=<{[0-1]-[0-1]-[0-1]}>=' _ _ 1
    =<{0-0-1}>=
    =<{0-1-1}>=
    =<{1-0-1}>=
    =<{1-1-1}>=

we need to write

    for value in pat.values((None, None, 1)):
        print(f"'{value}'")

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    '-<{0-0-1}>-'
    '-<{0-1-1}>-'
    '-<{1-0-1}>-'
    '-<{1-1-1}>-'


### Matching a string against a pattern

Assuming we have a pattern and one of its instances, for example `PCIeRetimer_[0-7]` and `PCIeRetimer_1`, we can obtain the list of index tuples at which `PCIeRetimer_[0-7]` evaluated would yield `PCIeRetimer_1` by using the `invert()` method of the `DeviceIdPattern` object.

    import ids

    pat = ids.parse("PCIeRetimer_[0-7]")

    invPat = pat.invert()
    print(list(invPat.values("PCIeRetimer_1")))

Output:

    $ PYTHONPATH=../device-id-lang python3 some-script.py
    [(1,)]

As usual, the `values(...)` method returns an iterator, so to list the values explicitly it must be passed to the `list` constructor.

The `invert()` method of `DeviceIdPattern` returns an object of type `InvDeviceIdPattern`. It's similar to `DeviceIdPattern`, but more limited. In particular it doesn't provide the `pairs()` method (yet).


# Quick intro to the device id pattern language C++ library

In this section the language is introduced alongside with the C++ library implementing it.


## Basic usage

The core of the library is the `DeviceIdPattern` class. In constructor it takes a string with an expression in device id mapping language, parses it, and provides high-level functions to extract information about the mapping.

    DeviceIdPattern gpus("GPU_SXM_[1-8]");

What the expression `"GPU_SXM_[1-8]"` represents is the following mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`GPU_SXM_8`</td>
</tr>
</tbody>
</table>

The arguments can be iterated with:

    for (const PatternIndex& index: gpus.domain())
    {
        std::cout << index << std::endl;
    }

Output:

    (1)
    (2)
    (3)
    (4)
    (5)
    (6)
    (7)
    (8)

The values can be iterated with:

    for (const std::string& name: gpus.values())
    {
        std::cout << name << std::endl;
    }

Output:

    GPU_SXM_1
    GPU_SXM_2
    GPU_SXM_3
    GPU_SXM_4
    GPU_SXM_5
    GPU_SXM_6
    GPU_SXM_7
    GPU_SXM_8

The whole pairs can be iterated in a map-like fashion with

    for (const auto& [index, name] : gpus)
    {
        std::cout << index << " -> " << name << std::endl;
    }

Output:

    (1) -> GPU_SXM_1
    (2) -> GPU_SXM_2
    (3) -> GPU_SXM_3
    (4) -> GPU_SXM_4
    (5) -> GPU_SXM_5
    (6) -> GPU_SXM_6
    (7) -> GPU_SXM_7
    (8) -> GPU_SXM_8

These constructs don't produce any intermediate collections. The variables `index`, `name` are calculated on demand. If we want convert them into concrete collections we can use the following functions:

    std::vector<device_id::PatternIndex> indexes = gpus.domainVec();
    std::vector<std::string> names = gpus.valuesVec();
    std::map<device_id::PatternIndex, std::string> indexesToNames = gpus.map();

The string value for a concrete argument can be obtained with the overloaded function call operator:

    std::cout << gpus(4) << std::endl;
    std::cout << gpus(8) << std::endl;

Output:

    GPU_SXM_4
    GPU_SXM_8

The `()` operator is a shorthand for the `eval(...)` function taking `PatternIndex` object as an argument. The following constructs are equivalent to the previous ones:

    std::cout << gpus.eval(PatternIndex(4)) << std::endl;
    std::cout << gpus.eval(PatternIndex(8)) << std::endl;

While it's more verbose in direct usage it can be useful when all we have is a `PatternIndex` object and not the literal index.

We can check whether a given string in variable `someDeviceName` matches the pattern with `matches(...)` function. Then to obtain the index to which it corresponds to we can use `match(...)`  function.

    if (gpus.matches(someDeviceName))
    {
        std::vector<PatternIndex> patternIndexes = gpus.match(someDeviceName);
        for (const auto& pIndex : patternIndexes)
        {
            std::cout << pIndex << std::endl;
        }
    }

For `someDeviceName` being `"GPU_SXM_5"` the output will be

    (5)

The reason `match(...)` returns a vector instead of a single `PatternIndex` object is because in general many arguments can produce the same value (this case will be covered later), as well as there can be none of them (if the `matches(...)` returns false and we call `match(...)` anyway).


## Coupling `match(...)` with `eval(...)`

The `match(...)` function works well together with `eval(...)` if we use the result of the former from one pattern to feed it to the latter of another pattern. For example:

    DeviceIdPattern gpus("GPU_SXM_[1-8]");
    DeviceIdPattern gpusDbusPaths(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]");

    std::vector<PatternIndex> gpuIndexes = gpus.match("GPU_SXM_3");
    for (const auto& gpuIndex: gpuIndexes)
    {
        std::cout << gpusDbusPaths.eval(gpuIndex) << std::endl;
    }

Output:

    /xyz/openbmc_project/inventory/system/processors/GPU_SXM_3

It's important that the domains of both patterns are the same. For `"GPU_SXM_[1-8]"` we have

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`GPU_SXM_8`</td>
</tr>
</tbody>
</table>

For `"/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8]"` we have

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_8`</td>
</tr>
</tbody>
</table>

so whatever `gpuIndex` we obtain from `gpus.match(...)` it is a valid input to `gpusDbusPaths.eval(...)`. However, if we defined the `gpusDbusPaths` like this (notice the indexes range)

    DeviceIdPattern gpusDbusPaths(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[0-7]");

then we have mis-aligned mappings

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">`gpus` values</th>
<th scope="col" class="org-left">`gpusDbusPaths` values</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0)</td>
<td class="org-left">&#xa0;</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_0`</td>
</tr>


<tr>
<td class="org-left">(1)</td>
<td class="org-left">`GPU_SXM_1`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`GPU_SXM_2`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`GPU_SXM_3`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`GPU_SXM_4`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`GPU_SXM_5`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`GPU_SXM_6`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`GPU_SXM_7`</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/processors/GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`GPU_SXM_8`</td>
<td class="org-left">&#xa0;</td>
</tr>
</tbody>
</table>

and the following code would throw an exception at `gpusDbusPaths.eval(gpuIndex)`

    DeviceIdPattern gpus("GPU_SXM_[1-8]");
    DeviceIdPattern gpusDbusPaths(
        "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[0-7]");

    std::vector<PatternIndex> gpuIndexes = gpus.match("GPU_SXM_8");
    for (const auto& gpuIndex: gpuIndexes)
    {
        std::cout << gpusDbusPaths.eval(gpuIndex) << std::endl;
    }

The point of this library though is to handle such misalignments. How it's done will be discussed later (see section [Index mappings](#org496b8d6)).


## Multiple brackets

The pattern provided to the `DeviceIdPattern` constructor can have more than one bracket. This is useful in handling devices which are identified in context of other devices, like NVLink. For example, we could identify NVLinks associeted with NVSwitch by using the following pattern:

    DeviceIdPattern switchNvlinks("NVSwitch_[0-3]/NVLink_[0-2]");

This corresponds to the following mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0, 0)</td>
<td class="org-left">`NVSwitch_0/NVLink_0`</td>
</tr>


<tr>
<td class="org-left">(0, 1)</td>
<td class="org-left">`NVSwitch_0/NVLink_1`</td>
</tr>


<tr>
<td class="org-left">(0, 2)</td>
<td class="org-left">`NVSwitch_0/NVLink_2`</td>
</tr>


<tr>
<td class="org-left">(1, 0)</td>
<td class="org-left">`NVSwitch_1/NVLink_0`</td>
</tr>


<tr>
<td class="org-left">(1, 1)</td>
<td class="org-left">`NVSwitch_1/NVLink_1`</td>
</tr>


<tr>
<td class="org-left">(1, 2)</td>
<td class="org-left">`NVSwitch_1/NVLink_2`</td>
</tr>


<tr>
<td class="org-left">(2, 0)</td>
<td class="org-left">`NVSwitch_2/NVLink_0`</td>
</tr>


<tr>
<td class="org-left">(2, 1)</td>
<td class="org-left">`NVSwitch_2/NVLink_1`</td>
</tr>


<tr>
<td class="org-left">(2, 2)</td>
<td class="org-left">`NVSwitch_2/NVLink_2`</td>
</tr>


<tr>
<td class="org-left">(3, 0)</td>
<td class="org-left">`NVSwitch_3/NVLink_0`</td>
</tr>


<tr>
<td class="org-left">(3, 1)</td>
<td class="org-left">`NVSwitch_3/NVLink_1`</td>
</tr>


<tr>
<td class="org-left">(3, 2)</td>
<td class="org-left">`NVSwitch_3/NVLink_2`</td>
</tr>
</tbody>
</table>

The number of NVlinks, usually around 40, was reduced for the sake of clarity. The ranges in brackets are treated independently, with the rightmost changing the fastest.

The object `switchNvlinks` can be used in the exact same manner as if it had just one bracket.

Iterating arguments:

    for (const PatternIndex& index: switchNvlinks.domain())
    {
        std::cout << index << std::endl;
    }

Output:

    (0, 0)
    (0, 1)
    (0, 2)
    (1, 0)
    (1, 1)
    (1, 2)
    (2, 0)
    (2, 1)
    (2, 2)
    (3, 0)
    (3, 1)
    (3, 2)

Iterating values:

    for (const std::string& name: switchNvlinks.values())
    {
        std::cout << name << std::endl;
    }

Output:

    NVSwitch_0/NVLink_0
    NVSwitch_0/NVLink_1
    NVSwitch_0/NVLink_2
    NVSwitch_1/NVLink_0
    NVSwitch_1/NVLink_1
    NVSwitch_1/NVLink_2
    NVSwitch_2/NVLink_0
    NVSwitch_2/NVLink_1
    NVSwitch_2/NVLink_2
    NVSwitch_3/NVLink_0
    NVSwitch_3/NVLink_1
    NVSwitch_3/NVLink_2

Iterating the whole map:

    for (const auto& [index, name] : switchNvlinks)
    {
        std::cout << index << " -> " << name << std::endl;
    }

Output:

    (0, 0) -> NVSwitch_0/NVLink_0
    (0, 1) -> NVSwitch_0/NVLink_1
    (0, 2) -> NVSwitch_0/NVLink_2
    (1, 0) -> NVSwitch_1/NVLink_0
    (1, 1) -> NVSwitch_1/NVLink_1
    (1, 2) -> NVSwitch_1/NVLink_2
    (2, 0) -> NVSwitch_2/NVLink_0
    (2, 1) -> NVSwitch_2/NVLink_1
    (2, 2) -> NVSwitch_2/NVLink_2
    (3, 0) -> NVSwitch_3/NVLink_0
    (3, 1) -> NVSwitch_3/NVLink_1
    (3, 2) -> NVSwitch_3/NVLink_2

This explains why the single indexes were surrounded by brackets before, like `(7)` - they were simply tuples of one element.

Constructing collections of the above:

    std::vector<device_id::PatternIndex> indexes = switchNvlinks.domainVec();
    std::vector<std::string> names = switchNvlinks.valuesVec();
    std::map<device_id::PatternIndex, std::string> indexesToNames = switchNvlinks.map();

Same issue with functions `eval(...)` and `match(...)`:

    DeviceIdPattern switchNvlinks("NVSwitch_[0-3]/NVLink_[0-2]");
    DeviceIdPattern gpusDbusPaths(
        "/xyz/openbmc_project/inventory/system/fabrics/"
        "HGX_NVLinkFabric_0/Switches/NVSwitch_[0-3]/Ports/NVLink_[0-2]");

    std::vector<PatternIndex> switchNvlinksIndexes = gpusDbusPaths.match(
        "/xyz/openbmc_project/inventory/system/fabrics/"
        "HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_2");
    for (const auto& index : switchNvlinksIndexes)
    {
        std::cout << index << std::endl;
        std::cout << switchNvlinks.eval(index) << std::endl;
    }

Output:

    (0, 2)
    NVSwitch_0/NVLink_2


## No-brackets string

An important case of the numbers of brackets in a pattern is when there is zero of them. This is already part of the provided mechanism and no special handling is needed. For example the pattern

    DeviceIdPattern bb("Baseboard");

represents the following mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">()</td>
<td class="org-left">`Baseboard`</td>
</tr>
</tbody>
</table>

The pattern of a string without any brackets will always have exactly one value equal to itself, obtained with evaluation without any arguments.

    std::cout << bb.domainVec()[0] << std::endl;
    std::cout << bb() << std::endl;
    std::cout << bb.eval(PatternIndex()) << std::endl;

Output:

    ()
    Baseboard
    Baseboard


<a id="org7a37884"></a>

## Dimensionality of id pattern and pattern index

The number of indexes in the `PatternIndex` object is its *dimension*. For example, the dimension of `(0, 3)` is 2. It's returned by the function `dim()`.

    PatternIndex x(0, 3);
    std::cout << "'" << x << "'.dim() = " << x.dim() << std::endl;

Output:

    '(0, 3)'.dim() = 2

The `DeviceIdPattern` class also uses the notion of *dimension* returned by function `dim()`. In the simplest case it's equal to the number of brackets.

    DeviceIdPattern pat("NVSwitch_[0-3]/NVLink_[0-2]");
    std::cout << "'" << pat << "'.dim() = '" << pat.dim() << std::endl;

Output:

    'NVSwitch_[0-3]/NVLink_[0-2]'.dim() = 2

In evaluating the pattern with `eval(...)` the dimension of pattern index must be no lesser than the dimension of the id pattern. In case it's greater the additional indexes are simply ignored.

    DeviceIdPattern pat("NVSwitch_[0-3]/NVLink_[0-2]");
    pat.eval(PatternIndex(0, 1));    // ok, => "NVSwitch_0/NVLink_1"
    pat.eval(PatternIndex(0, 1, 5)); // ok, => "NVSwitch_0/NVLink_1"
    pat.eval(PatternIndex(2));       // error
    pat.eval(PatternIndex());        // error

This is a simplified rule. The precise conditions pattern index must fulfill for the evaluation of id pattern to be successful will be explained in the following sections.


# Language details


## Specifying bracket mappings


<a id="org496b8d6"></a>

### Index span mapping

So far all the patterns shown used brackets in the form `[a-b]` which meant a sequence of numbers `a, a+1, ..., b` denoting the arguments of the pattern. However, this is just a reduced version of the form

    [a-b:c-d]

which denotes a mapping of arguments `a, a+1, ..., b` into values `c, c+1, ..., d`, where `b - a = d - c` (the number spans must be equal in length). Upon evaluation of the id pattern the argument from `a-b` is changed into corresponding value from `c-d` and only then inserted into surrounding text. For example, the id pattern `"HSC_[1-8:0-7]"` represents the following mapping

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`HSC_0`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`HSC_1`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`HSC_2`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`HSC_3`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`HSC_4`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`HSC_5`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`HSC_6`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`HSC_7`</td>
</tr>
</tbody>
</table>

This makes it possible to express shifted indexing in different domains.

The values span can also be a single number, in which case every number from the arguments span is mapped to this single value. For example the pattern `"HSC_[0-1:8]"` represents the mapping

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0)</td>
<td class="org-left">`HSC_8`</td>
</tr>


<tr>
<td class="org-left">(1)</td>
<td class="org-left">`HSC_8`</td>
</tr>
</tbody>
</table>

This allows for expressing associations of multiple devices with a single other device. In the given example that would be `NVSwitch_0` and `NVSwitch_1` associated with the same hot swap controller `HSC_8`.

To sum up, the following mapping syntax is allowed inside the bracket

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Syntax</th>
<th scope="col" class="org-left">Equivalent</th>
<th scope="col" class="org-left">Example</th>
<th scope="col" class="org-left">Meaning</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">`[a-b:c-d]`</td>
<td class="org-left">`[a-b:c-d]`</td>
<td class="org-left">`[0-7:1-8]`</td>
<td class="org-left">Map arguments `a, a+1, ..., b` to values `c, c+1, ..., d`</td>
</tr>


<tr>
<td class="org-left">`[a-b:c]`</td>
<td class="org-left">`[a-b:c-c]`</td>
<td class="org-left">`[0-1:8]`</td>
<td class="org-left">Map arguments `a, a+1, ..., b` to a single value `c`</td>
</tr>


<tr>
<td class="org-left">`[a-b]`</td>
<td class="org-left">`[a-b:a-b]`</td>
<td class="org-left">`[0-4]`</td>
<td class="org-left">Map arguments `a, a+1, ..., b` to themselves</td>
</tr>


<tr>
<td class="org-left">`[a:c]`</td>
<td class="org-left">`[a-a:c-c]`</td>
<td class="org-left">`[2:4]`</td>
<td class="org-left">Map argument `a` to value `c`</td>
</tr>


<tr>
<td class="org-left">`[a]`</td>
<td class="org-left">`[a-a:a-a]`</td>
<td class="org-left">`[1]`</td>
<td class="org-left">Map argument `a` to itself</td>
</tr>
</tbody>
</table>


<a id="orgf0ef5fe"></a>

### Mapping sums

The bracket in id pattern can specify multiple index span mappings, separated by commas:

    [a-b:c-d,e-f:g-h,...]

The resulting mapping is all of the separate mappings joined. For example the pattern `GPU_SXM_[0-3:5-8,4-7:1-4]` represents the following mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0)</td>
<td class="org-left">`GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(1)</td>
<td class="org-left">`GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`GPU_SXM_8`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`GPU_SXM_4`</td>
</tr>
</tbody>
</table>

This makes possible expressing the translation between `GPU_SXM_` indexed in a non-linear manner in Redfish and the GPUs indexed from `0` to `7` in SMBPBI.

Another useful example is the pattern `HSC_[0-1:8,2-3:9]`, representing the mapping

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0)</td>
<td class="org-left">`HSC_8`</td>
</tr>


<tr>
<td class="org-left">(1)</td>
<td class="org-left">`HSC_8`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`HSC_9`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`HSC_9`</td>
</tr>
</tbody>
</table>

This expresses the association between NVSwitches and hot swap controllers, where the NVSwitch device is represented implicitly by the index from `0` to `3`. How this logic can be used to provide explicit mapping between full device names will be covered in [Device id mapping configuration](#org2bdb76d).


<a id="org6c7b8c0"></a>

## Input position indexes


### Changing input order

The bracket syntax makes it possible to change the ordering of input indexes, so that, for example, given the pattern index `(3, 5)` the `5` went into the first bracket of the pattern, while `3` went into the second. This is achieved by providing the *input position index* `r` using the syntax.

    [r|...]

where `...` is an arbitrary mapping specification as described in [index span mappings](#org496b8d6) and [mapping sums](#orgf0ef5fe). The indexing of the input starts from 0. For example, the pattern:

    NVSwitch_[1|0-3]/NVLink_[0|10-12]

represents the following mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(10, 0)</td>
<td class="org-left">`NVSwitch_0/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(10, 1)</td>
<td class="org-left">`NVSwitch_1/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(10, 2)</td>
<td class="org-left">`NVSwitch_2/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(10, 3)</td>
<td class="org-left">`NVSwitch_3/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(11, 0)</td>
<td class="org-left">`NVSwitch_0/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(11, 1)</td>
<td class="org-left">`NVSwitch_1/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(11, 2)</td>
<td class="org-left">`NVSwitch_2/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(11, 3)</td>
<td class="org-left">`NVSwitch_3/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(12, 0)</td>
<td class="org-left">`NVSwitch_0/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(12, 1)</td>
<td class="org-left">`NVSwitch_1/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(12, 2)</td>
<td class="org-left">`NVSwitch_2/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(12, 3)</td>
<td class="org-left">`NVSwitch_3/NVLink_12`</td>
</tr>
</tbody>
</table>

Compare it with a similar pattern:

    NVSwitch_[0|0-3]/NVLink_[1|10-12]

which represents the mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(0, 10)</td>
<td class="org-left">`NVSwitch_0/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(0, 11)</td>
<td class="org-left">`NVSwitch_0/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(0, 12)</td>
<td class="org-left">`NVSwitch_0/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(1, 10)</td>
<td class="org-left">`NVSwitch_1/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(1, 11)</td>
<td class="org-left">`NVSwitch_1/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(1, 12)</td>
<td class="org-left">`NVSwitch_1/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(2, 10)</td>
<td class="org-left">`NVSwitch_2/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(2, 11)</td>
<td class="org-left">`NVSwitch_2/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(2, 12)</td>
<td class="org-left">`NVSwitch_2/NVLink_12`</td>
</tr>


<tr>
<td class="org-left">(3, 10)</td>
<td class="org-left">`NVSwitch_3/NVLink_10`</td>
</tr>


<tr>
<td class="org-left">(3, 11)</td>
<td class="org-left">`NVSwitch_3/NVLink_11`</td>
</tr>


<tr>
<td class="org-left">(3, 12)</td>
<td class="org-left">`NVSwitch_3/NVLink_12`</td>
</tr>
</tbody>
</table>

The feature is very useful in [event node instantiation](#orgee8834d), in which the brackets of the pattern (for example arguments to a command line program of some accessor) may appear in a different order than the instantiating pattern index. This syntax is also a basis for expressing [bound brackets](#orgb158aaf), that is different brackets in the same pattern which nevertheless iterate over the same span range.


<a id="orgb158aaf"></a>

### Bound brackets

The same input index can be repeated in multiple brackets, for example

    /xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[0|1-8]/PCIeDevices/GPU_SXM_[0|1-8]

In this case the corresponding brackets are said to be *bound*, which means they're always evaluated with the same arguments. The pattern above represents the mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_1/PCIeDevices/GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_2/PCIeDevices/GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_3/PCIeDevices/GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_4/PCIeDevices/GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_5/PCIeDevices/GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_6/PCIeDevices/GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_7/PCIeDevices/GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_8/PCIeDevices/GPU_SXM_8`</td>
</tr>
</tbody>
</table>

This is useful in representing strings where the same device index occurs in multiple places. The example presented is a real world example, a dbus object path for the accessor of the "PCIe Link Speed State Change" event.

    "accessor": {
      "type": "DBUS",
      "object": "/xyz/openbmc_project/inventory/system/chassis/HGX_GPU_SXM_[0|1-8]/PCIeDevices/GPU_SXM_[0|1-8]",
      "interface": "xyz.openbmc_project.Inventory.Item.PCIeDevice",
      "property": "PCIeType",
      "check": {
        "not_equal": "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen5"
      }
    }

All bound brackets must define the same *arguments set*. The values sets may be different, but arguments must be the same, because they are, in fact, not the arguments of the particular bracket, but of the associated *input position*, which is shared. For example, the pattern (real world example of arguments to the `fpga_regtbl_wrapper` in one of the `dat.json` entries)

    FPGA_SXM[0|1-8:0-7]_PWR_EN GPU_SXM_[0|1-8]

representing the mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`FPGA_SXM0_PWR_EN GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`FPGA_SXM1_PWR_EN GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`FPGA_SXM2_PWR_EN GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`FPGA_SXM3_PWR_EN GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`FPGA_SXM4_PWR_EN GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`FPGA_SXM5_PWR_EN GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`FPGA_SXM6_PWR_EN GPU_SXM_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`FPGA_SXM7_PWR_EN GPU_SXM_8`</td>
</tr>
</tbody>
</table>

is correct, because both brackets specify the same arguments span `1-8`, even if the first bracket maps it to the span `0-7` while the second maps it to the `1-8`. However, if the pattern was of the form

    FPGA_SXM[0|0-7]_PWR_EN GPU_SXM_[0|1-8]

it would be incorrect, because the first bracket specifies arguments span `0-7` while the second one specifies `1-8`. The `DeviceIdPattern` class doesn't raise any error upon construction of such pattern, but it issues a warning about reducing the domain of each bound bracket to the common subset. For the pattern above it would construct a mapping

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`FPGA_SXM0_PWR_EN GPU_SXM_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`FPGA_SXM1_PWR_EN GPU_SXM_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`FPGA_SXM2_PWR_EN GPU_SXM_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`FPGA_SXM3_PWR_EN GPU_SXM_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`FPGA_SXM4_PWR_EN GPU_SXM_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`FPGA_SXM5_PWR_EN GPU_SXM_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`FPGA_SXM6_PWR_EN GPU_SXM_7`</td>
</tr>
</tbody>
</table>

as the span `1-7` is the greatest common subset of `0-7` and `1-8`.


### Implicit input position indexes

When the input position index is omitted it's assumed to be equal to the position of the bracket in the pattern, so a pattern like:

    abc_[1,3,5]-def_[3-6]-ghi_[5-7]

is equivalent to:

    abc_[0|1,3,5]-def_[1|3-6]-ghi_[2|5-7]

This rule is applied to every bracket without an input position index independently of whether other brackets are indexed or not. In effect all of the following patterns are equivalent:

    abc_[1,3,5]-def_[3-6]-ghi_[5-7]
    abc_[1,3,5]-def_[3-6]-ghi_[2|5-7]
    abc_[1,3,5]-def_[1|3-6]-ghi_[5-7]
    abc_[1,3,5]-def_[1|3-6]-ghi_[2|5-7]
    abc_[0|1,3,5]-def_[3-6]-ghi_[5-7]
    abc_[0|1,3,5]-def_[3-6]-ghi_[2|5-7]
    abc_[0|1,3,5]-def_[1|3-6]-ghi_[5-7]
    abc_[0|1,3,5]-def_[1|3-6]-ghi_[2|5-7]

This rule also implies that some brackets may implicitly become bound (see [Bound brackets](#orgb158aaf)). For example consider the pattern:

    GPU_SXM_[1-8]/NVLink_[0|0-17]

It will NOT represent the mapping with the two brackets running independently, but being equivalent to

    GPU_SXM_[0|1-8]/NVLink_[0|0-17]

it will represent the mapping:

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(1)</td>
<td class="org-left">`GPU_SXM_1/NVLink_1`</td>
</tr>


<tr>
<td class="org-left">(2)</td>
<td class="org-left">`GPU_SXM_2/NVLink_2`</td>
</tr>


<tr>
<td class="org-left">(3)</td>
<td class="org-left">`GPU_SXM_3/NVLink_3`</td>
</tr>


<tr>
<td class="org-left">(4)</td>
<td class="org-left">`GPU_SXM_4/NVLink_4`</td>
</tr>


<tr>
<td class="org-left">(5)</td>
<td class="org-left">`GPU_SXM_5/NVLink_5`</td>
</tr>


<tr>
<td class="org-left">(6)</td>
<td class="org-left">`GPU_SXM_6/NVLink_6`</td>
</tr>


<tr>
<td class="org-left">(7)</td>
<td class="org-left">`GPU_SXM_7/NVLink_7`</td>
</tr>


<tr>
<td class="org-left">(8)</td>
<td class="org-left">`GPU_SXM_8/NVLink_8`</td>
</tr>
</tbody>
</table>

This behavior may be unexpected, it's therefore advisable to either omit the input position indexes entirely or to provide them explicitly for all the brackets.


### Input gaps

Specifying input position indexes allows for creating patterns which have "gaps" in the input, that is input positions with which no bracket is associated. For example the pattern

    PCIeRetimer_[1|0-7]

has no bracket associated with input position 0. Such construct is perfectly valid. Because the device id pattern language is expected to be used not only to specify the device ranges but also to instantiate arbitrary strings parameterized with the device numbers (eg. accessor's dbus object path), it's desirable to be able to pick an arbitrary input position for a particular bracket and ignore the rest.

In instantiating the `PCIeRetimer_[1|0-7]` pattern the provided `PatternIndex` object needs to have the dimension of at least 2. Whatever value appears on the first position will be ignored similarly to how indexes at higher dimensions than that of a pattern are ignored (see [Dimensionality of id pattern and pattern index](#org7a37884)).

    DeviceIdPattern pat("PCIeRetimer_[1|0-7]");
    pat.eval(PatternIndex(11, 5));     // ok, => "PCIeRetimer_5"
    pat.eval(PatternIndex(13, 5));     // ok, => "PCIeRetimer_5"
    pat.eval(PatternIndex(13, 5, 3));  // ok, => "PCIeRetimer_5"
    pat(13, 5, 3);      // same as above, ok, => "PCIeRetimer_5"
    pat.eval(PatternIndex(5));         // error
    pat();                             // error

In representing the full mapping a special format of `_` is used in place of the ignored input positions, denoting "anything". The mapping represented by the `PCIeRetimer_[1|0-7]` pattern is thus

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Argument</th>
<th scope="col" class="org-left">Value</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">(&#95;, 0)</td>
<td class="org-left">`PCIeRetimer_0`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 1)</td>
<td class="org-left">`PCIeRetimer_1`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 2)</td>
<td class="org-left">`PCIeRetimer_2`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 3)</td>
<td class="org-left">`PCIeRetimer_3`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 4)</td>
<td class="org-left">`PCIeRetimer_4`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 5)</td>
<td class="org-left">`PCIeRetimer_5`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 6)</td>
<td class="org-left">`PCIeRetimer_6`</td>
</tr>


<tr>
<td class="org-left">(&#95;, 7)</td>
<td class="org-left">`PCIeRetimer_7`</td>
</tr>
</tbody>
</table>

This representation format is supported by the library.

    DeviceIdPattern pat("PCIeRetimer_[1|0-7]");
    for (const PatternIndex& pIndex: pat.domain())
    {
        std::cout << pIndex << std::endl;
    }

Output:

    (_, 0)
    (_, 1)
    (_, 2)
    (_, 3)
    (_, 4)
    (_, 5)
    (_, 6)
    (_, 7)

To construct the `PatternIndex` object with `_` directly we can use the constant `PatternIndex::unspecified`:

    std::cout << PatternIndex(PatternIndex::unspecified, 5) << std::endl;

Output:

    (_, 5)

The `PatternIndex::unspecified` can be provided in places ignored by the pattern without any error

    DeviceIdPattern pat("PCIeRetimer_[1|0-7]");
    pat.eval(PatternIndex(PatternIndex::unspecified,
                          5));                        // ok, => "PCIeRetimer_5"
    pat(PatternIndex::unspecified, 5); // same as above, ok, => "PCIeRetimer_5"
    pat(PatternIndex::unspecified,                    // error, position '1'
        PatternIndex::unspecified);                   //   must have a specified value


## Complete device id pattern language syntax

    string_map: string? ( brack_map string? )*

    brack_map          : "[" indexed_map "]"
    indexed_map        : sum_map | proper_indexed_map
    proper_indexed_map : number "|" sum_map
    sum_map            : map ( "," map )*
    map                : range | proper_map
    proper_map         : range ":" range
    range              : number | proper_range
    proper_range       : number "-" number

    number: /[0-9]+/
    string: /[^[]+/


<a id="org2bdb76d"></a>

# Device id mapping configuration


# Device id patterns in `event_info.json`


<a id="orgee8834d"></a>

## Event node instantiation


# Json generation

