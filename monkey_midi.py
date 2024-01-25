import mido

# midi preparation:
# midi length should be 8, 16, 32 16th notes
# remove all the notes sounding in parallel
# ideally keep them more or less 16th, or your sequencer will go polyrhythmic
# also, note end events are ignored, only note starts matter

# then paste the result to the arduino code, replacing "[]" with "{}", intead of pattern1 or pattern2

file_name = "monkey_drummer_a_hats_plus.MID"

def note_lengths(midi_file_path):
    mid = mido.MidiFile(midi_file_path)
    ticks_per_beat = mid.ticks_per_beat
    ticks_per_16th_note = ticks_per_beat / 4

    lengths = []

    # Process each track
    for i, track in enumerate(mid.tracks):
        print(f'Track {i}: {track.name}')

        cumulative_time = 0  # Initialize cumulative time
        last_note_start_in_16th = 0  # Initialize the last note start time in 16th note fractions

        for msg in track:
            # Add the delta time to the cumulative time
            cumulative_time += msg.time

            # Check if the message is a 'note on' event and the velocity is not zero
            if msg.type == 'note_on' and msg.velocity > 0:
                note_start_in_16th = cumulative_time / ticks_per_16th_note
                diff_in_16th = note_start_in_16th - last_note_start_in_16th
                last_note_start_in_16th = note_start_in_16th

                # prettier printing
                diff_in_16th = round(diff_in_16th, 3)

                print(f'Note start in 16th note fractions: {note_start_in_16th}, Diff from last note: {diff_in_16th}')
                lengths += [ diff_in_16th ]
    # last node should make the total length the same as without groove
    last_length = len(lengths) - sum(lengths)
    if (last_length < 0):
        print ("Last note has a negative length, what's wrong with your midi?")
    lengths += [last_length]
    return lengths[1:]

lengths = note_lengths(file_name)

print (lengths)
print (f"Total notes (should be 8, 16, 32...) : {len(lengths)}")
print (f"Total pattern length (should be 8, 16, 32...) : {sum(lengths)}")
