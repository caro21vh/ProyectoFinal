# Sistema de Maceta Inteligente
---
## Integrantes
- Marisa Méndez Mesén - 2021123104
- Carolina Montero Shum - 2022437825
- Diana Obando Chacón - 2022017493
- Carolina Vargas Hidalgo - 2022093527
- Albert Vega Camacho - 2021048094
---

Este proyecto consiste en un ecosistema de **Maceta Inteligente** diseñado para automatizar y optimizar el cuidado de plantas en entornos urbanos o interiores. El sistema realiza un monitoreo en tiempo real de variables ambientales esenciales (humedad del suelo, intensidad lumínica y nivel del tanque de agua) y toma decisiones automatizadas de control (activación de luz de cultivo o alertas sonoras/visuales) para mantener la planta saludable. Se compone de un dispositivo físico basado en el microcontrolador **ESP32** y una **interfaz web** interactiva.

---

## Arquitectura del Sistema

El dispositivo físico se estructura verticalmente en tres secciones estratégicas para garantizar estabilidad mecánica y seguridad eléctrica:
1. **Sección Superior (Electrónica y Alertas):** Resguarda el microcontrolador ESP32, la tira LED NeoPixel y el Buzzer. Al estar aislada en la parte más alta, se elimina el riesgo de cortocircuitos por fugas de agua.
2. **Sección Central (Entorno Biológico):** Alberga la planta, el sensor de luz (BH1750) y el higrómetro FC-28 para medir la humedad de la tierra.
3. **Sección Inferior (Depósito de Agua):** Actúa como tanque de reserva de agua e incluye un sensor de nivel de agua. Mantener el peso del agua en la base funciona como un contrapeso natural que evita volteos.

---
## Instrucciones de montaje
1. Asegúrese de que la estructura física esté completa e íntegra, debe incluir las tres secciones de la estructura anteriormente mencionadas.
2. Ingrese suficiente cantidad de agua en el depósito de agua inferior y asegúrese de que la cuerda quede sumergida en esta.
3. Coloque la maceta con su planta en el contenedor central de la estructura.
4. Introduzca el higómetro dentro de la tierra de la maceta.
5. Por último coloque la tapa de la circuitería superior para finalizar con el montaje.


## Requisitos de Instalación

### 1. Componentes de Hardware (incluidos en el sistema)
* **Microcontrolador:** ESP32 (con conectividad Wi-Fi integrada).
* **Sensores:**
  * Higrómetro de suelo FC-28.
  * Sensor de nivel de agua.
  * Sensor de luz BH1750.
* **Actuadores y Alertas:**
  * Tira de LEDs NeoPixel.
  * Buzzer.
* **Alimentación:**
  * Batería recargable de 3.7V para alimentar el ESP32.

### 2. Software para la Aplicación Web
La interfaz de usuario es una aplicación web estática cliente-servidor construida con tecnologías web estándar. **No requiere instalación de servidores locales (como Node.js o Apache)**.
* Requisitos: Un navegador web moderno (Google Chrome, Mozilla Firefox, Microsoft Edge o Safari).

---

## Configuración y Puesta en Marcha

### 1. Funcionamiento Autónomo (Dispositivo Físico)
El ESP32 toma mediciones cada 200 ms y evalúa las alertas según el perfil de planta configurado. El comportamiento de las luces NeoPixel y del Buzzer varía según los problemas detectados:

- **Estado Normal**: Las luces muestran un patrón de monitoreo estándar y la planta está segura.
- **Tierra Seca**: El buzzer emite un sonido intermitente para avisar que el suelo necesita hidratación.
- **Bajo Nivel de Luz**: La tira LED NeoPixel se enciende en modo Grow Light (luz de cultivo morada/rosada) para compensar la radiación solar ausente.
- **Alerta Combinada (Luz Baja + Tierra Seca)**: Cuando la luz de cultivo está encendida para compensar la iluminación y, simultáneamente, la tierra se seca, el sistema activa un parpadeo rápido de advertencia cada 3 segundos en la tira LED para indicar visualmente que se debe revisar el riego/tanque urgentemente.
- **Tanque Vacío**: Se genera un pitido recurrente y una luz de advertencia para alertar al usuario de rellenar el depósito antes de que la bomba sufra daños o se quede sin agua para regar.

### 2. Uso de la Aplicación Web
- Localice el archivo interfaz_app.html en su computadora.
- Haga doble clic sobre él para abrirlo en cualquier navegador de internet.
- Al cargar la interfaz, verá un campo de entrada en la parte superior que dice "IP del ESP32".
- Ingrese la dirección IP del ESP32 y haga clic en ``Conectar``.
- **Monitoreo en Vivo:** Una vez establecida la conexión, la app cambiará su estado a ``CONECTADO`` (icono verde pulsante) y comenzará a actualizar en tiempo real los valores de:
  - Humedad del Suelo (porcentaje de 0% a 100%).
  - Intensidad Lumínica.
  - Estado del Tanque (Indicador visual de ``Tanque con agua`` o ``Tanque vacío — rellenar depósito``).

- **Selección de Perfil de Planta:** En la interfaz web se listan botones interactivos para diferentes especies:
  - Suculenta, Helecho, Cactus, Tomate, Albahaca, Genérica, etc.
  - Al seleccionar una planta, la interfaz envía un comando HTTP POST (/plant) al ESP32. El microcontrolador cambiará instantáneamente los umbrales de sequedad y luz mínima de acuerdo con las necesidades específicas de la especie seleccionada, actualizando la lógica de riego y luces del hardware de forma inmediata.
- **Consola de Registro (Log):** En la parte inferior de la pantalla, una consola detallada irá registrando cronológicamente los eventos, cambios de planta y errores de conexión para facilitar el diagnóstico del sistema.
