from veiculo import Veiculo

class Moto(Veiculo):
    def __init__(self,modelo,fabricante,rodas,descricao,cilindrada):
       super().__init__(modelo,fabricante,rodas,descricao)

       self.__cilindrada=cilindrada

    def get_cilindrada(self):
        return self.__cilindrada
    def set_portas(self,cilindrada):
        self.__cilindrada=cilindrada
    
    def imprime(self):
        super().imprime()
        print(f"Cilindradas:{self.__cilindrada}")
     
    def emite_som(self):
        print(f"Som da moto:HANTANTATAN HAN HANTANNNNN!!!!!!!!")