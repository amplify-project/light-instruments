#ifndef CONFIG_H
#define CONFIG_H

#ifdef XIAO
#define DIP1 D3
#define DIP2 D4
#define DIP3 D5

#define SELECTOR1 D0
#define SELECTOR2 D1
#define SELECTOR3 D2

#define DATA D8
#endif

#ifdef LOLIN
#define DIP1 25
#define DIP2 26
#define DIP3 27

#define SELECTOR1 16
#define SELECTOR2 17
#define SELECTOR3 18

#define DATA 4
#endif

#endif
