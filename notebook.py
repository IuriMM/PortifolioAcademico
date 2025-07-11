from random import randint

class Notebook:
    def __init__(self):
        self.__notes = list()

    def storeNote(self,note):
        self.__notes.append(note)

    def numberOfNotes(self):
        return len(self.__notes)

    def showNote(self,noteNumber):
        if noteNumber < 0:
            print("Este não é um número de nota válido")
        elif noteNumber < self.numberOfNotes():
            print(self.__notes[noteNumber])
        else :
            print("Este não é um número de nota válido")

    def removeNote(self, note):
        if note in self.__notes:
            self.__notes.remove(note)
            print("Nota removida com sucesso")
        else:
            print("Esta não é uma nota válida")
    
    def listNotes(self):
        index = 0
        if self.numberOfNotes() == 0:
            print("Não tem notas na lista")
            return 0
        while index < self.numberOfNotes():
            print(self.__notes[index])
            index += 1

    #Atividade 3: listNotesfor
    def listNotesfor(self):
        for note in self.__notes:
            print(note)

    #Atividade 4: hasNotes
    def hasNotes(self):
        if(self.numberOfNotes() == 0):
            return 0
        else: return 1

    #Atividade 5: compareNote
    def compareNote(self,note):
        for notes in self.__notes:
            if(notes == note):
                return 1
        return 0
    
    #Atividade 6: showNoteRandom
    def showNoteRandom(self):
        if self.numberOfNotes() == 0:
            return None
        n = randint(0,self.numberOfNotes() - 1)
        return self.__notes[n]

    #Atividade 7: showMultiNoteRandom
    def showMultiNoteRandom(self, quantidade):
        if self.numberOfNotes() == 0:
            return None
        if quantidade <= 0:
            return None
        total = self.numberOfNotes()
        count = 0
        while count < quantidade:
            n = randint(0, total - 1)
            print(self.__notes[n])
            count += 1
