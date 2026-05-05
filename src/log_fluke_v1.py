import serial
import csv
import time 
# CONFIGURAÇÕES HARD CODED
# Tempo em segundos entre as medições
INTERVALO_SECONDS = 1  

def iniciar_log_separado():
    # --- ENTRADAS DO USUÁRIO ---
    porta = input("Porta COM (ex: COM5): ").strip().upper()    
    nome_arq = input("Nome do arquivo CSV: ").strip()
    if not nome_arq.lower().endswith('.csv'): nome_arq += '.csv'
   
    try:
        ser = serial.Serial(porta, 9600, timeout=1)
        print(f"\n[LENDO] LENDO {nome_arq} da {porta}...")

        with open(nome_arq, mode='a', newline='') as file:
            # Definimos o delimitador como ponto e vírgula para o Excel abrir direto em colunas
            writer = csv.writer(file, delimiter=';') 
            
            if file.tell() == 0:
                writer.writerow(['Data','Horário', 'Tensao (V)', 'Corrente (A)'])
            ser.write(b'MEAS?\r\n') 
            time.sleep (0.5)
            while True:
                ser.write(b'MEAS?\r\n') 
                leitura_bruta = ser.readline().decode('utf-8').strip()
                if "=>" in leitura_bruta: 
                    pass
                else:
                    if leitura_bruta:
                        # timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                        data_atual = time.strftime('%Y-%m-%d')
                        tempo_atual = time.strftime('%H:%M:%S')

                        time.sleep (0.5)
                        partes = leitura_bruta.split(',')
                    
                        if len(partes) >= 2:
                            corrente_bruta = partes[0].replace("ADC","").strip()
                            tensao_bruta   = partes[1].replace("VDC","").strip()

                            corrente_limpa = "".join(c for c in corrente_bruta if c in "0123456789.+-E")
                            tensao_limpa =   "".join(c for c in tensao_bruta if c in "0123456789.+E")
                            if corrente_limpa and tensao_limpa:
                                 try:
                                    corrente_float = float(corrente_limpa) 
                                    tensao_float = float(tensao_limpa)
                                    corrente_excel = str(corrente_float).replace('.',',')
                                    tensao_excel = str(tensao_float).replace('.',',')
                                    writer.writerow([data_atual,tempo_atual, tensao_excel, corrente_excel])
                                    print(f"[{data_atual} {tempo_atual}]  V: {tensao_excel} | A: {corrente_excel}")
                                 except ValueError:
                                       print(f"[{tempo_atual}] Erro de leitura (dados corrompidos): {leitura_bruta}")  
                        else:
                        # Se o multímetro mandar só um valor, grava na primeira coluna
                            writer.writerow([data_atual,tempo_atual, partes[0].strip(), "---"])
                            print(f"[{data_atual} {tempo_atual}] Leitura única: {partes[0]}")
                    
                        file.flush()
                
                        time.sleep(INTERVALO_SECONDS)

    except KeyboardInterrupt:
        print("\nGravação finalizada.")
    finally:
        if 'ser' in locals(): ser.close()

if __name__ == "__main__":
    iniciar_log_separado()