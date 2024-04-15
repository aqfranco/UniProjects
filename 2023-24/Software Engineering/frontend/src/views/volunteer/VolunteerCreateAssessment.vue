<template v-if="assessment">
  <v-dialog
    v-model="dialog"
    @input="$emit('close-dialog')"
    @keydown.esc="$emit('close-dialog')"
    max-width="75%"
    max-height="80%"
  >
    <v-card>
      <v-form ref="form" lazy-validation>
        <v-card-title>
          <span class="headline">New Assessment</span>
        </v-card-title>

        <v-card-text class="text-left">
          <v-text-field
            v-model="assessment.review"
            label="*Review"
            :rules="[(value) => !!value || 'Review is required']"
            required
            data-cy="reviewInput"
          />
        </v-card-text>

        <v-card-actions>
          <v-spacer />
          <v-btn
            color="blue darken-1"
            @click="$emit('close-dialog')"
            data-cy="cancelButton"
            >Close</v-btn
          >
          <v-btn
            v-if="!(assessment.review.length < 10)"
            color="blue darken-1"
            @click="submit"
            data-cy="saveButton"
          >
            Save</v-btn
          >
        </v-card-actions>
      </v-form>
    </v-card>
  </v-dialog>
</template>

<script lang="ts">
import { Vue, Component, Model, Prop } from 'vue-property-decorator';
import Assessment from '@/models/assessment/Assessment';
import RemoteServices from '@/services/RemoteServices';
import Institution from '@/models/institution/Institution';

@Component({})
export default class VolunteerCreateAssessment extends Vue {
  @Model('dialog', Boolean) dialog!: boolean;
  @Prop({ type: Institution, required: true })
  readonly institution!: Institution;
  valid = [];
  success = false;
  assessment: Assessment = new Assessment();

  async created() {
    this.assessment = new Assessment();
    this.assessment.review = '';
  }

  async submit() {
    this.success = false;

    if (!(this.$refs.form as Vue & { validate: () => boolean }).validate()) {
      return;
    }

    const result = await RemoteServices.createAssessment(
      this.institution.id,
      this.assessment,
    );
    this.$emit('assessment-created', result);
    this.success = true;
  }
}
</script>

<style scoped lang="scss"></style>
