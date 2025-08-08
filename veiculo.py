class Veiculo:
    def __init__(self, modelo, fabricante, rodas, descricao):
        self.__modelo = modelo
        self.__fabricante = fabricante
        self.__rodas = rodas
        self.__descricao = descricao

    def get_modelo(self):
        return self.__modelo

    def set_modelo(self, modelo):
        self.__modelo = modelo

    def get_fabricante(self):
        return self.__fabricante

    def set_fabricante(self, fabricante):
        self.__fabricante = fabricante

    def get_rodas(self):
        return self.__rodas

    def set_rodas(self, rodas):
        self.__rodas = rodas

    def get_descricao(self):
        return self.__descricao

    def set_descricao(self, descricao):
        self.__descricao = descricao

    def imprime(self):
        print(f"Modelo: {self.__modelo}")
        print(f"Fabricante: {self.__fabricante}")
        print(f"Rodas: {self.__rodas}")
        print(f"Descrição: {self.__descricao}")

    def emite_som(self):
        pass
