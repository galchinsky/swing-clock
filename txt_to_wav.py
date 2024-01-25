import soundfile as sf

def text_to_stereo_wav(input_text_file, output_wav_file, sample_rate=1000):
    # Read the text file
    with open(input_text_file, 'r') as file:
        lines = file.readlines()

    # Convert lines to a list of tuples (stereo data)
    data = [tuple(map(float, line.strip().split())) for line in lines]

    # Write to a WAV file as stereo
    sf.write(output_wav_file, data, sample_rate)

# Usage
text_to_stereo_wav('a.txt', 'stereo_output.wav', 1000)

