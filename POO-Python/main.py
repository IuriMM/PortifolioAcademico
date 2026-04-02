from notebook import Notebook

def main():
    notas = Notebook()

    #Atividade 2: input
    Nota = ""
    print("Escreva as notas a serem adicionadas, digite parar para interromper o loop:")
    while True:
        Nota = input()
        if Nota == "parar":
            break
        notas.storeNote(Nota)
    
    #teste de todos os metodos
    print("\nLista das notas:")
    notas.listNotesfor()
    print("\nChecagem se há notas(1 se tiver, 0 se não tiver):")
    print(notas.hasNotes())
    print("\nVerificar se há nota igual a Lanchar e Estudar programação, respectivamente:")
    print(notas.compareNote("Lanchar"))
    print(notas.compareNote("Estudar programação"))
    print("\nMostrar nota aleatoria:")
    print(notas.showNoteRandom())
    print("\nMostrar 3 Notas aleatorias")
    notas.showMultiNoteRandom(3)
    print("\nMostrar a 3 nota, se existir:")
    notas.showNote(2)
    print("\nRemover a nota Estudar, se existir:")
    notas.removeNote("Estudar")
    print("\nMostrando notas após a remoção:")
    notas.listNotes()

main()