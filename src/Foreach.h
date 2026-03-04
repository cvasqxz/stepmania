#ifndef Foreach_H
#define Foreach_H

/* C++11 modernization: use auto for iterator types */
#define FOREACH( elemType, vect, var ) 			\
for( auto var = (vect).begin(); var != (vect).end(); ++var )
#define FOREACH_CONST( elemType, vect, var ) 	\
for( auto var = (vect).cbegin(); var != (vect).cend(); ++var )

#define FOREACHD( elemType, vect, var ) 			\
for( auto var = (vect).begin(); var != (vect).end(); ++var )
#define FOREACHD_CONST( elemType, vect, var ) 	\
for( auto var = (vect).cbegin(); var != (vect).cend(); ++var )

#endif
