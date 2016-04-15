
##Current Features:

####Identifier based

| command | args | full name | description |
|----------|------|--------|------------|
#r | target match | Replace | Replaces 'target' with 'match'


####For Loops

```
 #fl //abbreviation of forlines
  ...
 #efl(text to paste) //endforlines
```


| command | args | full name | description |
| ------- | ---- | --------- | ----------- |
|  #l     |   NONE   |  Line     |  Pates the current line |
|  #lc    | (x, y) | LineClip | Inserts the line with x chars removed from the front and y chars removed from the back
| #w      | (i) | Word | Inserts the identifier at the provided index |
  

---

##Features in Progress:


| command | args | full name | description |
|----------|------|--------|------------|
#rw  | target match | Replace World | Replaces the target word with match
#d   | target | Delete    | Delets the target
#dw  | target | Delete Word | Deletes the target word
#ptr | target | ToPointer | Changes target to use pointer semantics
#val | target |  ToValue  | Changes target to use value semantics
