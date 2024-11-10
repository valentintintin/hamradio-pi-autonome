#!/bin/bash

#set -e # Stop le programme s'il y a une erreur
set -x # Affiche les commandes

# Paramètres et variables
SERIAL_PORT="/dev/ttyS1"                # Port série pour la communication
SERIAL_BAUDRATE="115200"                    # Débit en bauds
PHOTO_OUTPUT_DIR="/data"            # Dossier de sortie des photos
REMOTE_DIR="valentin@192.168.1254:/home/valentin/Data/cameras/opi"  # Dossier distant pour rsync
SLEEP_DURATION=150                        # Temps de veille (en secondes) si Ethernet non connecté

# Fonction pour setup la carte
startup() {
    echo "Setup de la carte"
    
    # cpu
    cpufreq-set --governor ondemand
    
    # configure UART
    config-pin p9.24 uart
    config-pin p9.26 uart
    stty -F /dev/ttyS1 115200

    # configure leds
    echo cpu0 > /sys/class/leds/beaglebone\:green\:usr0/trigger
    echo none > /sys/class/leds/beaglebone\:green\:usr1/trigger
    echo none > /sys/class/leds/beaglebone\:green\:usr2/trigger
    echo none > /sys/class/leds/beaglebone\:green\:usr3/trigger
    
    echo 0 > /sys/class/leds/beaglebone\:green\:usr0/brightness
    echo 0 > /sys/class/leds/beaglebone\:green\:usr1/brightness
    echo 0 > /sys/class/leds/beaglebone\:green\:usr2/brightness
    echo 0 > /sys/class/leds/beaglebone\:green\:usr3/brightness
}

# Fonction pour lire et analyser le JSON depuis le port série
read_json_from_serial() {
    echo "Envoi de la commande sur le port série..."
    echo -e "COMMAND\n" > "$SERIAL_PORT" # Remplacez "COMMAND" par la commande réelle
    JSON_RESPONSE=$(cat "$SERIAL_PORT")  # Lecture de la réponse JSON

    # Vérification que la réponse est bien du JSON
    if echo "$JSON_RESPONSE" | jq . > /dev/null 2>&1; then
        echo "JSON reçu: $JSON_RESPONSE"

        # Output the JSON
        echo "$JSON_RESPONSE" > "$PHOTO_OUTPUT_DIR/mcu.json"
    else
        echo "Erreur : Réponse non valide"
    fi
}

# Fonction pour régler l'heure système depuis un timestamp JSON
set_system_time() {
    TIMESTAMP=$(echo "$JSON_RESPONSE" | jq '.date') # Extraction du timestamp
    echo "Réglage de l'heure du système avec le timestamp : $TIMESTAMP"
    sudo date -s "@$TIMESTAMP"
    sudo hwclock -w -f /dev/rtc0
}

# Fonction pour capturer une photo avec chaque caméra
capture_photos() {
    echo "Capture des photos avec fswebcam pour chaque caméra..."
    for CAMERA_CONFIG in /path/to/camera_configs/*.conf; do
        CAMERA_NAME=$(basename "$CAMERA_CONFIG" .conf)
        TIMESTAMP=$(date +"%Y-%m-%d-%H-%M-%S")
        PHOTO_PATH="$PHOTO_OUTPUT_DIR/${CAMERA_NAME}_${TIMESTAMP}.jpg"

        fswebcam -c "$CAMERA_CONFIG" --save "$PHOTO_PATH"
        echo "Photo capturée pour la caméra $CAMERA_NAME : $PHOTO_PATH"
    done
}

# Fonction pour générer une image de télémétrie
generate_telemetry_image() {
    TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
    BATTERY_VOLTAGE=$(echo "$JSON_RESPONSE" | jq '.voltageBattery')
    BATTERY_CURRENT=$(echo "$JSON_RESPONSE" | jq '.currentBattery')
    PANEL_VOLTAGE=$(echo "$JSON_RESPONSE" | jq '.voltageSolar')
    PANEL_CURRENT=$(echo "$JSON_RESPONSE" | jq '.currentSolar')
    TEMPERATURE=$(echo "$JSON_RESPONSE" | jq '.temperature')
    HUMIDITY=$(echo "$JSON_RESPONSE" | jq '.humidity')
    PRESSURE=$(echo "$JSON_RESPONSE" | jq '.pressure')

    OUTPUT_FILE="$PHOTO_OUTPUT_DIR/telemetry_${TIMESTAMP}.jpg"

    # Exemple de commande ImageMagick, ajustez les variables si nécessaire
    convert -size 700x480 xc:black \
        -font "/path/to/Arial.ttf" -pointsize 30 -stroke white -strokewidth 1 -fill white \
        -annotate +185+30 "$TIMESTAMP" \
        -fill lightgray -stroke lightgray -draw "rectangle 85,60 615,85" \
        -fill indianred -stroke indianred -draw "rectangle 85,60 85,85" \
        -font "/path/to/Arial.ttf" -pointsize 20 -stroke black -strokewidth 1 -fill black \
        -annotate +135+80 "Tension batterie : ${BATTERY_VOLTAGE} mV" \
        -annotate +15+80 "11000" -annotate +620+80 "13500" \
        -annotate +135+130 "Courant batterie : ${BATTERY_CURRENT} mA" \
        -annotate +15+130 "0" -annotate +620+130 "1000" \
        -annotate +135+180 "Tension panneau : ${PANEL_VOLTAGE} mV" \
        -annotate +15+180 "0" -annotate +620+180 "25000" \
        -annotate +135+230 "Courant panneau : ${PANEL_CURRENT} mA" \
        -annotate +15+230 "0" -annotate +620+230 "4000" \
        -annotate +135+280 "Température : ${TEMPERATURE} °C" \
        -annotate +15+280 "-10" -annotate +620+280 "50" \
        -annotate +135+330 "Humidité : ${HUMIDITY} %" \
        -annotate +15+330 "0" -annotate +620+330 "100" \
        -annotate +135+380 "Pression : ${PRESSURE} hPa" \
        -annotate +15+380 "950" -annotate +620+380 "1050" \
        -quality 40 -format jpeg "$OUTPUT_FILE"

    echo "Image de télémétrie générée : $OUTPUT_FILE"
}

# Fonction pour vérifier si l'Ethernet est connecté
check_ethernet_connection() {
    if ip link show | grep "state UP"; then
        return 0  # Connecté
    else
        return 1  # Non connecté
    fi
}

# Fonction pour synchroniser les images vers le dossier distant
sync_photos() {
    echo "Synchronisation des photos vers le dossier distant..."
    rsync -avz "$PHOTO_OUTPUT_DIR/" "$REMOTE_DIR"
}

save_system_info() {
    # Get raw CPU percentage
    cpu_percentage=$(top -bn1 | grep "%Cpu(s)" | awk '{printf "%.0f", $2}')

    # Get RAM percentage
    ram_percentage=$(free | grep Mem | awk '{printf "%.0f", (1 - $7/$2) * 100}')

    # Get disk usage percentage
    disk_percentage=$(df -h / | awk 'NR==2 {print $5}' | cut -d'%' -f1)

    # Get uptime in seconds
    uptime_seconds=$(awk '{printf "%.0f", $1}' /proc/uptime)

    # Build JSON output
    json="{ \"cpu\": $cpu_percentage, \"ram\": $ram_percentage, \"disk\": $disk_percentage, \"uptime\": $uptime_seconds }"

    # Output the JSON
    echo "$json" > "$PHOTO_OUTPUT_DIR/system.json"
}

startup

# Boucle principale
while true; do
    read_json_from_serial            # Lire et traiter le JSON reçu via le port série
    set_system_time                  # Régler l'heure système à partir du JSON
    capture_photos                   # Capturer les photos de chaque caméra
    #generate_telemetry_image         # Générer l'image de télémétrie
    save_system_info                 # Récupère l'état de la carte

    if check_ethernet_connection; then
        sync_photos                  # Si connecté, synchroniser les photos
        sleep $SLEEP_DURATION
    else
        echo "Ethernet non connecté. On attend 10 secondes avant de dormir $SLEEP_DURATION secondes..."
        sleep 10
        rtcwake -d /dev/rtc0 -m mem -s "$SLEEP_DURATION"      # Sinon, attendre avant de recommencer
    fi
done

