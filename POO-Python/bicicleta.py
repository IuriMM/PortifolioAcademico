from veiculo import Veiculo

class Bicicleta(Veiculo):
    def __init__(self, modelo, fabricante, rodas, descricao, marchas):
        super().__init__(modelo, fabricante, rodas, descricao)
        self.__marchas= marchas

    def get_marchas(self):
        return self.__marchas

    def set_portas(self, marchas):
        self.__marchas = marchas

    def imprime(self):
        super().imprime()
        print(f"Marchas: {self.__marchas}")

    def emite_som(self):
        print("Som da bike: TRIM TRIM!!!!!!!!")