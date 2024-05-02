# Programming Assignment 1 - TreePipe

## Intro

Create a binary tree of processes with the `in-order` ordering (left, self, right) using fork(), execvp(), pipe(), dup2(), read(), write() and close().

## Usage

Create `left` and `right` by modifying the OPERATION macro inside `p.c`

```C
// add => 0
// multiply => 1
// subtract => 2
// addSubtract => 3
// minimum => 4
// maximum => 5
// bitwiseAND => 6
// divideByTwo => 7

#define OPERATION 0 // add
```

then run:

```bash
> gcc p.c -o left -Wall
```

to create `left`

and

```bash
> gcc p.c -o right -Wall
```

to create `right`

Once both `left` and `right` have been created, run the program with:

```bash
> ./treePipe <current depth> <max depth> <left-right>
```

## Sample Usage

```bash
> ./treePipe 0 2 0
```

Output:

```bash
> current depth: 0, lr: 0
Please enter num1 for the root: 2
> my num1 is: 2
---> current depth: 1, lr: 0
---> my num1 is: 2
------> current depth: 2, lr: 0
------> my num1 is: 2
------> my result is: 3
---> current depth: 1, lr: 0, my num1: 2, my num2: 3
---> my result is: 5
------> current depth: 2, lr: 1
------> my num1 is: 5
------> my result is: 5
> current depth: 0, lr: 0, my num1: 2, my num2: 5
> my result is: 7
---> current depth: 1, lr: 1
---> my num1 is: 7
------> current depth: 2, lr: 0
------> my num1 is: 7
------> my result is: 8
---> current depth: 1, lr: 1, my num1: 7, my num2: 8
---> my result is: 56
------> current depth: 2, lr: 1
------> my num1 is: 56
------> my result is: 56
The final result is: 56
```
