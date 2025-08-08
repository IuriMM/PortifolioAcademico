from veiculo import Veiculo

class Carro(Veiculo):
    def __init__(self, modelo, fabricante, rodas, descricao, portas):
        super().__init__(modelo, fabricante, rodas, descricao)
        self.__portas = portas

    def get_portas(self):
        return self.__portas

    def set_portas(self, portas):
        self.__portas = portas

    def imprime(self):
        super().imprime()
        print(f"Portas: {self.__portas}")

    def emite_som(self):
        print("Som do carro: VRUM VRUM!!!!!!!!")
